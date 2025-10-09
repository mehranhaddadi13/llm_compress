import pprint
from typing import List
import torch
import torch.nn as nn
from transformers import AutoTokenizer, AutoModelForCausalLM, TextDataset, DataCollatorForLanguageModeling, Trainer, TrainingArguments, get_linear_schedule_with_warmup
from peft import LoraConfig, get_peft_model
from utils.finetune_utils import CastOutputToFloat, print_trainable_parameters
from utils.monitor import monitor
from huggingface_hub import login
from transformers import BitsAndBytesConfig
from os.path import basename

def ft(model, save_path, dataset_path="../datasets/enwik/enwik9_100m.txt", stat_path='stats/ftstats.csv', block_size=128, epochs=10, r=8,
              learning_rate=1e-4, batch_size=64):
    """Finetune the given LLM on the given dataset.
    
    Arguments:
    model -- The name of the LLM
    save_path -- The path to save the finetuned LLM at
    Keyword Arguments:
    dataset_path -- The path to the dataset the LLM will be finetuned on
    stat_path -- The path to save the finetuning statistics
    block_size -- The maximum sequence length contained in each training example; passed to TextDataset
    epochs -- The number of epochs; passed to TrainingArguments
    r -- The LoRA rank; passed to LoraConfig
    learning_rate -- The finetuning's learning rate; passed to TrainingArguments
    batch_size -- The size of each batch used for finetuning; passed to TrainingArguments"""
    
    # Load the LLM in 4bits quantization.
    loaded_model = AutoModelForCausalLM.from_pretrained(
        model, 
        load_in_4bit=True,
	    device_map ="auto"
    )

    # Load the tokenizer of the LLM.
    tokenizer = AutoTokenizer.from_pretrained(model)

    print("Model loaded")

    for param in loaded_model.parameters():
        param.requires_grad = False  # freeze the model - train adapters later
        if param.ndim == 1:
            param.data = param.data.to(torch.float32)

    loaded_model.gradient_checkpointing_enable()  
    loaded_model.enable_input_require_grads()

    loaded_model.lm_head = CastOutputToFloat(loaded_model.lm_head)    

    config = LoraConfig(
        r=r,
        lora_alpha=32, 
        lora_dropout=0.05,
        bias="none",
        task_type="CAUSAL_LM"
    )

    loaded_model = get_peft_model(loaded_model, config)
    print_trainable_parameters(loaded_model)

    loaded_model.to(torch.cuda.current_device())

    dataset = TextDataset(
        tokenizer=tokenizer,
        file_path=dataset_path,
        block_size=block_size
    )

    data_collator = DataCollatorForLanguageModeling(
        tokenizer=tokenizer, mlm=False
    )

    print("Dataset loaded")

    training_args = TrainingArguments(
        per_device_train_batch_size=batch_size, 
        gradient_accumulation_steps=8,
        max_steps=epochs, 
        learning_rate=learning_rate, 
        fp16=True,
        logging_steps=1,
        output_dir="output",
        warmup_steps=500,
        weight_decay=0.01,
        max_grad_norm=1.0,
    )

    trainer = Trainer(
        model=loaded_model, 
        train_dataset=dataset,
        args=training_args,
        data_collator=data_collator
    )

    loaded_model.config.use_cache = False

    # Run finetuning through 'monitor' and obtain the finetuning's statistics.
    stats = [basename(dataset_path)] + monitor(True, trainer.train)
    # Insert the obtained statistics to the statistics file.
    with open(stat_path, 'a') as f:
        f.write('\n' + ','.join(stats))
    trainer.save_model(save_path + f"_{epochs}_r{r}") 

    del loaded_model
    del trainer
    torch.cuda.empty_cache()

    print("Finished finetuning")

# if __name__ == "__main__":
#     login("HF LOG IN TOKEN HERE!")
#     finetune_list = [
#        "meta-llama/Llama-3.2-1B"
#     ]
#     epoch_list = [256]

#     for model in finetune_list:
#         for e in epoch_list:
#             ft(
#                 model, 
#                 save_path=f"~/finetuned_models/{model.replace('/', '-')}-enwik100mb", 
#                 dataset_path="../datasets/enwik/enwik9_100m.txt", 
#                 block_size=128, 
#                 epochs=e, 
#                 r=8,
#                 learning_rate=1e-4, 
#                 batch_size=64
#             )