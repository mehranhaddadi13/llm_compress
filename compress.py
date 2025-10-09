import gzip, zlib, bz2, lzma, pyppmd
import os
from huggingface_hub import login
from finetune import ft
from llm_encode import compress as llm_compress
from trad_encode import Compressor

# Obtainin the path to datasets stored in 'datasets' directory.
datasets = []
for (root, directories, files) in os.walk('datasets'):
    if not directories:
        for f in files:
            # if f.endswith('00m.txt'):
             datasets.append(os.path.join(root, f))

# Creating 'stats' directory to store statistics of each run.
if not os.path.exists('stats'):
    os.mkdir('stats')
# Create a CSV file to store finetuning statistics.
if not os.path.exists('stats/ftstats.csv'):
    with open('stats/ftstats.csv', 'w') as f:
        f.write('data,time,gpu avg,gpu peak,gpu mem,cpu avg,cpu peak,mem,')
# Make a directory to save finetuned models in.
if not os.path.exists('finetuned_models'):
    os.mkdir('finetuned_models')
# Make a directory to save compressed files in.
if not os.path.exists('zipped'):
    os.mkdir('zipped')

# Login to HuggingFace through a token to access the model.
login("HF LOGIN TOKEN HERE!")
# Create a dictionary of traditional algorithms with their parameters set to achieve the highest compression.
trad_models = {gzip: {'compresslevel': 9}, zlib: {'level': 9}, bz2: {'compresslevel': 9}, lzma: {'preset': 9}, pyppmd: {}}
# The name of the LLM used.
llm_model = "meta-llama/Llama-3.2-1B"
# Number of epochs.
epoch = 256
# The LoRA rank.
r = 8

for dataset in datasets:
    finetuned_model = f"{llm_model.replace('/', '-')}-{os.path.basename(dataset).removesuffix('.txt')}"
    # Finetune the LLM on the given dataset.
    ft(
         llm_model, 
         save_path= f"finetuned_models/{finetuned_model}",
         dataset_path=dataset,
         stat_path = 'stats/ftstats.csv',
         block_size=128,
         epochs=epoch, 
         r=r,
         learning_rate=1e-4, 
         batch_size=64
        )
    # Make a file to store the statistics of each compression.
    c_stat_path = f'stats/{os.path.basename(dataset)}_cstats.csv'
    if not os.path.exists(c_stat_path):
        with open(c_stat_path, 'w') as f:
            f.write('model,time,gpu avg,gpu peak,gpu mem,cpu avg,cpu peak,mem,original size,compressed size,cr,ranks_0_15,ranks_0,')
    finetuned_models = {
        f"{finetuned_model}_{epoch}_r{r}": llm_model
    }

    context_sizes = [512]

    # Compress the data with the normal and finetuned LLM.
    llm_compress(finetuned_models=finetuned_models, original_model_names=[llm_model], context_sizes=context_sizes, batch_size=64, file_path=dataset,
                stat_path=c_stat_path)

    # Compress the data with each traditional algorithm.
    for (model, kwargs) in trad_models.items():
                compressor = Compressor(model, dataset, stat_path=c_stat_path)
                compressor.compress(**kwargs)