import json
import os
import bz2
import pprint
from typing import List
import numpy as np
import torch
from transformers import AutoTokenizer, AutoModelForCausalLM, BitsAndBytesConfig
import array
import zlib
from utils.monitor import monitor
 
import matplotlib.pyplot as plt
from tqdm import tqdm
import time
import os
from itertools import cycle

def tensor_to_numpy(tensor):
    conv = np.array(tensor.cpu().numpy(), dtype=np.uint32, order='C')
    for i in range(len(conv)):
        conv[i] = np.uint32(conv[i])
    return conv

def numpy_to_tensor(array: np.ndarray, device):
    array = array.astype(np.float32)
    return torch.tensor(array, device=device)

class ZipModel():
    def __init__(self, model_name, tokenizer_name, model, tokenizer, finetuned, context_size, batch_size):
        self.CONTEXT_SIZE = context_size  # originally 512
        self.BATCH_SIZE = batch_size      # originally 10
        self.tokenizer_name = tokenizer_name
        self.model_name = model_name
        self.model = model
        self.tokenizer = tokenizer
        self.finetuned = finetuned
        
        self.device = torch.cuda.current_device()
        # self.model.to(self.device)
        self.model.eval()

        self.ranks = []


    def text_to_tokens(self, text):
        tokens = self.tokenizer(text, return_tensors="pt")
        tokens = tokens["input_ids"].squeeze()
        return tokens

    def tokens_to_text(self, tokens):
        tokens = tokens.reshape((1, -1))
        text = self.tokenizer.batch_decode(tokens)
        return text[0]

    def pad(self, tokens, padding_val):
        pad_len = self.CONTEXT_SIZE - tokens.shape[0] % self.CONTEXT_SIZE
        if pad_len != self.CONTEXT_SIZE:
            padding = torch.tensor([padding_val]*pad_len)

            tokens = torch.cat((tokens, padding))

        else:
            pad_len = 0

        return tokens, pad_len

    @torch.no_grad()
    def get_logits(self, tokens, token_index, past=None):
        my_inputs = {}
        my_inputs['input_ids'] = tokens[:, token_index].reshape(-1, 1)

        output = self.model(**my_inputs, past_key_values=past)
        logits = output.logits
        if len(logits.shape) > 2:
            logits = logits.reshape((logits.shape[0], -1))
        return logits, output.past_key_values


    def encode_one_batch(self, tokens, token_index, past=None):

        assert len(tokens.shape) == 2

        logits, past = self.get_logits(tokens, token_index, past)
        assert len(logits.shape) == 2
        logits, sorted_tokens = torch.sort(logits, descending=True)

        assert len(sorted_tokens.shape) == 2

        next_tokens = tokens[:, token_index + 1]
        next_tokens_expanded = next_tokens.view(-1, 1).expand_as(sorted_tokens)

        # Find score as index of next tokens
        scores = (sorted_tokens==next_tokens_expanded).nonzero(as_tuple=True)

        scores = scores[1] # remove index column

        self.ranks.extend(scores.tolist())

        return scores, past

    def decode_one_batch(self, input_tokens, scores, score_index, past=None):
        assert len(scores.shape) == 2
        logits, past = self.get_logits(input_tokens, score_index, past)

        logits, sorted_tokens = torch.sort(logits, descending=True)
        assert len(sorted_tokens.shape) == 2
        # the scores give the indexes of the decoded tokens
        indexes = scores[:, score_index].flatten()
        decoded_tokens = sorted_tokens[torch.arange(indexes.shape[0]), indexes]

        return decoded_tokens.int(), past


    def encode(self, text):
        tokens = self.text_to_tokens(text)
        return self.encode_tokens(tokens)

    def encode_tokens(self, tokens):

        tokens, pad_len = self.pad(tokens, self.tokenizer.eos_token_id)
        tokens = tokens.view(-1, self.CONTEXT_SIZE)

        output_scores = torch.zeros(tokens.shape)

        # Add eos to the start of each block (to give it somewhere to start)
        eos = torch.tensor([self.tokenizer.eos_token_id]*tokens.shape[0]).unsqueeze(1)
        tokens = torch.cat((eos, tokens), 1)

        tokens = tokens.to(self.device)

        batches = tokens.shape[0]//self.BATCH_SIZE
        if tokens.shape[0] % self.BATCH_SIZE != 0:
            batches += 1

        # score each batch
        print("Encoding")
        for i in range(batches):
            cur_tokens = tokens[i*self.BATCH_SIZE:(i + 1)*self.BATCH_SIZE]
            cur_output_scores = torch.zeros((cur_tokens.shape[0], cur_tokens.shape[1]-1))
            past = None
            print(i, "out of", batches)

            for j in range(cur_tokens.shape[1]-1):
                cur_output_scores[:, j], past = self.encode_one_batch(cur_tokens, j, past)
                
            output_scores[i*self.BATCH_SIZE:(i + 1)*self.BATCH_SIZE] = cur_output_scores
            del cur_tokens

        torch.cuda.empty_cache()

        output_scores = output_scores.flatten().int()
        if pad_len > 0:
            output_scores = output_scores[:-pad_len]
        return output_scores

    def decode(self, scores):
        output_tokens = self.decode_tokens(scores)
        text = self.tokenizer.batch_decode(output_tokens)
        text = "".join(text)
        #text = text.replace("<|endoftext|>", "")
        return text

    def decode_tokens(self, scores):

        scores, pad_len = self.pad(scores, self.tokenizer.eos_token_id)

        scores = scores.view(-1, self.CONTEXT_SIZE) # all rows, CONTEXT_SIZE

        output_tokens = torch.zeros(scores.shape, dtype=int)

        # Add eos to the start of each block (to give it somewhere to start)
        eos = torch.tensor([self.tokenizer.eos_token_id]*output_tokens.shape[0]).unsqueeze(1)
        output_tokens = torch.cat((eos, output_tokens), 1) # all rows, CONTEXT_SIZE + 1

        output_tokens = output_tokens.to(self.device)

        batches = scores.shape[0]//self.BATCH_SIZE
        if scores.shape[0] % self.BATCH_SIZE != 0:
            batches += 1

        # score each batch
        print("Decoding")
        for i in range(batches):
            print(i, "out of", batches)
            cur_scores = scores[i*self.BATCH_SIZE:(i + 1)*self.BATCH_SIZE] # BATCH_SIZE, CONTEXT_SIZE

            cur_output_tokens = output_tokens[i*self.BATCH_SIZE:(i + 1)*self.BATCH_SIZE] # BATCH_SIZE, CONTEXT_SIZE
            cur_output_tokens = cur_output_tokens.to(self.device)
            past = None
            for j in tqdm(range(scores.shape[1])):

                cur_output_tokens[:, j+1], past = self.decode_one_batch(cur_output_tokens, cur_scores, j, past) # BATCH_SIZE

            output_tokens[i*self.BATCH_SIZE:(i + 1)*self.BATCH_SIZE] = cur_output_tokens

        output_tokens = output_tokens[:, 1:].int()
        output_tokens = output_tokens.flatten()

        if pad_len != 0:
            output_tokens = output_tokens[:-pad_len]

        return output_tokens

    def encode_and_zip(self, text):
        encoded = self.encode(text)
        tensor = torch.tensor(encoded)

        # Convert the tensor to bytes
        tensor_bytes = tensor.numpy().tobytes()

        # Compress the tensor bytes using bz2
        compressed_bytes = bz2.compress(tensor_bytes)

        return compressed_bytes

    def unzip_and_decode(self, zipped):
        unzipped = zlib.decompress(zipped)
        unzipped = array.array("H", unzipped)
        decoded = self.decode(torch.tensor(unzipped))
        return decoded

    def zip_file(self, text_file, zip_file):
        with open(text_file, encoding="utf-8") as f:
            text = f.read()

        zipped = self.encode_and_zip(text)
         
        #open(zip_file, 'wb').close()
        with open(zip_file, "wb") as f:
            f.write(zipped)

    def unzip_file(self, zip_file, text_file):
        with open(zip_file, "rb") as f:
            zipped = f.read()
        text = self.unzip_and_decode(zipped)

        with open(text_file, "w", encoding="utf-8") as f:
            f.write(text)

    def plot_rank_distribution(self, plot_type='histogram'):
        if plot_type == 'histogram':
            plt.figure(figsize=(10, 6))
            plt.hist(self.ranks, bins=100, log=True)
            plt.title('Distribution of Token Ranks')
            plt.xlabel('Rank')
            plt.ylabel('Frequency (log scale)')
        elif plot_type == 'scatter':
            plt.figure(figsize=(10, 6))
            # Create a scatter plot where x is the index of the rank and y is the rank value
            x = list(range(len(self.ranks)))  # Indexes of the ranks
            y = self.ranks  # The rank values
            plt.scatter(x, y, alpha=0.5)
            plt.title('Scatter Plot of Token Ranks')
            plt.xlabel('Sequence Position')
            plt.ylabel('Rank')
            plt.yscale('log')  # Using a log scale for the y-axis may help visualize large rank values
        else:
            print("Invalid plot type specified. Please choose 'histogram' or 'scatter'.")

        plt.show()
        
def compress(finetuned_models=None, original_model_names=None, context_sizes=[32], batch_size=64, file_path="../datasets/hindi/hindi_100m.txt",
              stat_path='stats/cstats.csv'):
    """Compress the given data with the given finetuned and original LLMs.
    
    Keyword Arguments:
    finetuned_models -- A dictionary of the name of finetuned models as keys and base models as values
    original_model_names -- A list of the original models
    context_sizes -- A list of the context sizes to be considered while calculating probabilties for the next token
    batch_size -- The batch size to be used
    file_path -- The path to the data that will be compressed
    stat_path -- The path to the file storing statistics of the execution's resource usage"""

    models = set()
    tokenizers = set()
    if finetuned_models is None and original_model_names is None:
        raise ValueError("model_dirs and original_model_names cannot both be None")
    
    if finetuned_models is None:
        finetuned_models = []
    if original_model_names is None:
        original_model_names = []
    
    for model in finetuned_models:
        models.add(model)
        tokenizers.add(finetuned_models[model])
    for model in original_model_names:
        models.add(model)
        tokenizers.add(model)

    # convert sets to lists
    models = list(models)
    tokenizers = list(tokenizers)

    print(models) 
    print(tokenizers) 

    zip_models: List[ZipModel] = []
    stats = {}

    # Load models
    for i in range(len(models)):
        model = models[i]
        if model in finetuned_models:
            loaded_model = AutoModelForCausalLM.from_pretrained(f"finetuned_models/{model}", device_map="auto", load_in_4bit=True)
            loaded_tokenizer = AutoTokenizer.from_pretrained(finetuned_models[model])
            tokenizer_name = finetuned_models[model]
        else:
            loaded_model = AutoModelForCausalLM.from_pretrained(model, device_map="auto", load_in_4bit=True)
            loaded_tokenizer = AutoTokenizer.from_pretrained(model)
            tokenizer_name = model

        # Create a ZipModel instance and compress the data through 'monitor' function to get the execution's statistics for each context size.
        for context_size in context_sizes:
            zip_model = ZipModel(model, tokenizer_name, loaded_model, loaded_tokenizer, model in finetuned_models, context_size, batch_size)
            zip_models.append(zip_model)
            stats[zip_model.model_name] = monitor(True, zip_model.zip_file, file_path, f"./zipped/{zip_model.model_name.replace('/', '-')}_{zip_model.CONTEXT_SIZE}_{zip_model.BATCH_SIZE}.gpz")

            # save ranks of every zip model in a text file with same name in 'zipped' directory.
            with open(f"zipped/{zip_model.model_name.replace('/', '-')}_{zip_model.CONTEXT_SIZE}_{zip_model.BATCH_SIZE}.txt", "w") as f:
                for rank in zip_model.ranks:
                    f.write(f"{rank}\n")
            
        
        del model
        torch.cuda.empty_cache()
    
    # for each model, get the % of ranks that are 0 and between 0-15 and the compression ratio.
    for zip_model in zip_models:
        # iterate over zip_models and for each zip_model, iterate over the ranks and make a new list only containing ranks between 0-15 and another containing ranks equal to 0. then divide the length of these lists by the total number of ranks and multiply by 100
        ranks_0_15 = []
        ranks_0 = []
        for rank in zip_model.ranks:
            if rank >= 0 and rank <= 15:
                ranks_0_15.append(rank)
                if rank == 0:
                    ranks_0.append(rank)
        ranks_0_15_per = (len(ranks_0_15)/len(zip_model.ranks))*100
        ranks_0_per = (len(ranks_0)/len(zip_model.ranks))*100
        compressed_size = os.path.getsize(f"zipped/{zip_model.model_name.replace('/', '-')}_{zip_model.CONTEXT_SIZE}_{zip_model.BATCH_SIZE}.gpz")
        dataset_size = os.path.getsize(file_path)
        compression_ratio = compressed_size/dataset_size
        stat = stats[zip_model.model_name]
        # Add the obtained statistics to the related file.
        stat = [f'\n{zip_model.model_name}'] + stat + [f"{dataset_size} B", f"{compressed_size} B", f"{compression_ratio:.6f}", f"{ranks_0_15_per:.2f} %", f"{ranks_0_per:.2f} %"]
        with open(stat_path, 'a') as f:
            f.write(','.join(stat))

# if __name__ == "__main__":

#     finetuned_models = {
#         "meta-llama-Llama-3.2-1B-hindi100mb_256_r8": "meta-llama/Llama-3.2-1B"
#     }

#     original_model_names = None# ["meta-llama/Llama-3.2-1B"]  # name of huggingface models
#     context_sizes = [512]

#     compress(finetuned_models=finetuned_models, original_model_names=original_model_names, context_sizes=context_sizes, batch_size=64, file_path="../datasets/hindi/hindi_100m.txt", save_name="1bit")