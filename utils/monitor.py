from time import sleep, perf_counter
import os, psutil, GPUtil
import threading
import torch

def monitor(llm=False, func=None, *args, **kwargs):
    """Execute a function and monitor its resource consumption.
    
    Keyword Arguments:
    llm -- If the function is from an LLM or not
    func -- The function to be monitored
    *args, **kwargs -- The arguments and keyword arguments to be passed to the function
    
    Output:
    result -- The output of the function
    stats -- The statistics of the resource consumption of the function"""

    # Create lists to record CPU and GPU usages.
    cpu_stats, gpu_stats = [], []

    def sampler():
        """Samples CPU load and GPU load and memory usage of the running function every 0.5 second in a separate thread."""
        
        while not stop:
            gpus = GPUtil.getGPUs()
            gpu_stats.append([(gpu.load, gpu.memoryUsed) for gpu in gpus])
            cpu_stats.append(process.cpu_percent(interval=None) / psutil.cpu_count())
            sleep(0.5)

    # Get the process ID of the current process.
    process = psutil.Process(os.getpid())

    # Start the sampler in a separate thread.
    stop = False
    t = threading.Thread(target=sampler)
    t.start()

    # Get the overall memory usage before executing the function.
    before_m = process.memory_full_info().uss
    # Record the starting time of the execution.
    start = perf_counter()
    # Run the funciton
    if llm:
        # LLM compression doesn't return anything.
        func(*args, **kwargs) 
    else:
        # Traditional algorithms return the compressed input.
        result = func(*args, **kwargs)
    # Record the end time of the execution.
    end = perf_counter()
    # Get the overall memory usage after the execution.
    after_m = process.memory_full_info().uss
    # Wait for GPU work to finish.
    torch.cuda.synchronize()
    # Stop the sampler function and join the other thread.
    stop = True
    t.join()

    # Calculating the execution time.
    time = f"{end - start:.2f} s"
    gpus_avg, gpus_peak, gpus_mem = [], [], []
    for i in range(len(gpu_stats[0])):
        # Calculate the average GPU load and memory usage of each GPU.
        gpu_loads = [s[i][0] for s in gpu_stats]
        gpu_mems = [s[i][1] for s in gpu_stats]
        gpus_avg.append(sum(gpu_loads)/len(gpu_loads))
        gpus_peak.append(max(gpu_loads))
        gpus_mem.append(max(gpu_mems))
    # Calculate the overall average of GPU load and memory usage and CPU load.
    gpu_avg = f"{(sum(gpus_avg)/len(gpus_avg)) * 100:.2f} %"
    gpu_peak = f"{max(gpus_peak) * 100:.2f} %"
    gpu_mem = f"{max(gpus_mem):.2f} MB"
    cpu_avg = f"{sum(cpu_stats)/len(cpu_stats):.2f} %"
    cpu_peak = f"{max(cpu_stats):.2f} %"
    # Calculating the memory usage.
    mem = f"{(after_m - before_m) / (1024 * 1024):.2f} MB"
    stats = [time,gpu_avg,gpu_peak,gpu_mem,cpu_avg,cpu_peak,mem]
    if llm:
        return stats
    else:
        return result, stats