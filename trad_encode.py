
import os
from utils.monitor import monitor
# import gzip, zlib, bz2, lzma, pyppmd


class Compressor:
    """The compressor class to compress the given data with traditional algorithms and record the statistics of the compression."""

    def __init__(self, alg, file, stat_path):
        self.alg = alg  # The traditional algorithm to be used.
        self.file = file    # The path to the dataset.
        self.stat = stat_path   # The path to the file to store the statistics in.
        self.file_size = os.path.getsize(file)

    def compress(self, **kwargs):
        """Compress the given file with the given algorithm and store the statistics of the compression.
        
        **kwargs -- Keyword arguments to be passed to be used by the compression algorithm."""
        
        with open(self.file, 'rb') as f:
            text = f.read()
        comp, stats = monitor(False, self.alg.compress, text, **kwargs) # Execute and monitor the compression and obtain the compressed data and the compression's statistics.
        self.comp_file = f"zipped/{os.path.basename(self.file)}_{self.alg.__name__}_compressed" # Write the compressed data to a file.
        with open(self.comp_file, 'wb') as c:
            c.write(comp)
        compressed_size = os.path.getsize(self.comp_file)
        cr = compressed_size / self.file_size   # Calculate the compression rate(compressed size/original size).
        # Adds the statistics to the related file. Note! The last two columns of the statistics file belong to the success rate of the
        # LLM to predict tokens correctly(ranks_0_15 and ranks_0) and are not applicable to traditional algorithms.
        stats = [f'\n{self.alg.__name__}'] + stats + [f"{self.file_size} B", f"{compressed_size} B", f"{cr:.6f}", "NA", "NA"]
        with open(self.stat, 'a') as f:
            f.write(','.join(stats))

# if __name__ == '__main__':
#     datasets = []
#     for (root, directories, files) in os.walk('datasets'):
#         if not directories:
#             for f in files:
#              datasets.append(os.path.join(root, f))
#     models = {pyppmd: {}, gzip: {'compresslevel': 9}, zlib: {'level': 9}, bz2: {'compresslevel': 9}, lzma: {'preset': 9}}
#     for data in data_list:
#     		for (model, kwargs) in models.items():
#                    compressor = Compressor(model, data)
#                    compressor.compress(**kwargs)
