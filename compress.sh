#!/bin/bash
# Making directories needed later in the code.
mkdir stats tawa_logs zipped models
# Create a CSV file with specified column headers to store statistics of each training process.
echo "model,time,cpu_avg,mem_max" > stats/tstats.csv

# A function to monitor the resource usage of each training and compression process.
monitor () {
	# Create an empty logfile for each model to store the relevant records of resource usage.
	logfile="tawa_logs/logs_${m}.txt"
	> "$logfile"
	# A variable to store the elapsed time, average CPU usage, and peak memory usage of each process.
	cstats=0
#	 # Execute the passed command through "time" command and write the output to the logfile.
#	 /usr/bin/time -v -o $logfile "$@"
#	 cpu=$(awk NR==4 $logfile | awk '{print $7}' | tr -d '%' | awk -v nproc=$(nproc --all) '{printf $1/nproc}')
#	 t=$(awk NR==5 $logfile | awk '{print $8}' | tr -d ':')
#	 mem=$(awk NR==10 $logfile | awk '{print $6}' | awk '{printf $1/1024}')
	# Execute the command passed to the monitor function (either training or compression) in background.
	"$@" &
	# Get the process ID of the command to be monitored.
	p=$!
	# While the command is running, write its elapsed time, CPU usage and memory usage to the logfile every half second.
	while ps -p $p > /dev/null
	do
		ps -p $p -o etimes,%cpu,rss --no-headers >> "$logfile"
		sleep 0.5
	done
	# cpu_max=0.0
	cpu_sum=0.0
	mem_max=0
	t=0
	# A variable to store the number of records in the logfile.
	inum=0
	# The number of CPU cores which is used later to obtain the CPU usage of each process.
	cores_num=$(nproc --all)
	# For each record in the log file:
	while read -r l
	do
		# Read each record.
		stats=($l)
		# Add one to the number of records.
		((inum++))
		# The elapsed time.
		t=${stats[0]}
		# The CPU usage.
		cpu=${stats[1]}
		# Devide the CPU usage by the number of cores to avoid over 100% CPU usages.
		cpu_n=$(echo "scale=2; ${cpu} / ${cores_num}" | bc -l)
		# Store the memory usage in MB.
		mem=$(echo "scale=2; ${stats[2]} / 1024" | bc -l)
#		if (( $(echo "${cpu_n} > ${cpu_max}" | bc -l) ));: then
#			cpu_max=${cpu_n}
#		fi
		# Add the CPU usage to the sum variable to be used to obtain the average CPU usage.
		cpu_sum=$(echo "scale=2; ${cpu_sum} + ${cpu_n}" | bc -l)
		# Update the value of the maximum memory usage.
		if (( $(echo "${mem} > ${mem_max}" | bc -l) )); then
			mem_max=${mem}
		fi
	done < "$logfile"
	# Avoid the zero division error if the logfile is empty.
	if (( inum == 0 )); then
		inum=1
	fi
	# Calculate the average CPU usage.
	cpu_avg=$(echo "scale=2; ${cpu_sum} / ${inum}" | bc -l)
	# Store statistics.
	cstats="${t},${cpu_avg},${mem_max}"
	#cstats="${t},${cpu},${mem}"
	
}

# Store the paths to datasets in "addresses" file.
find datasets -iname "*.txt" > addresses

# For each dataset address:
while read -r data
do
	# Obtain the dataset size to be used to calculate the compression ratio.
	data_size=$(stat -c%s "$data")
	# Get the dataset name from its address.
	File=$data
	arrFile=(${File//\// })
	file_name=${arrFile[-1]}
	# Write the column headers to the CSV file storing the statistics of compressing the dataset.
	echo "model,time,cpu_avg,mem_max,data_size,comp_size,cr" > stats/${file_name}.csv
	# Make a separate directory for Tawa models trained on the dataset.
	mkdir models/${file_name}
	# The name of the static model trained on the dataset.
	m="${file_name}_ts"
	# Train a static model with alphabet size of 256, PPM order of 5, and the escape probability estimation method set to D, through "monitor" function.
	monitor Tawa-0.7/apps/train/train -i "${data}" -o "models/${file_name}/tawa_${file_name}_static" -e D -a 256 -O 5 -T "${file_name} Static Trainer" -S
	# Write the training statistics of the static model to the relevant CSV file.
	printf "${m},${cstats}\n" >> stats/tstats.csv
	# The name of the dynamic model trained on the dataset.
	m="${file_name}_td"
	# Train a dynamic model with alphabet size of 256, PPM order of 5, and the escape probability estimation method set to D, through "monitor" function.
	monitor Tawa-0.7/apps/train/train -i "${data}" -o "models/${file_name}/tawa_${file_name}_dynamic" -e D -a 256 -O 5 -T "${file_name} Dynamic Trainer"
	# Write the training statistics of the dynamic model to the relevant CSV file.
	printf "${m},${cstats}\n" >> stats/tstats.csv

	# Store the addresses of the models trained on the dataset to "m_addresses" file.
	find models/${file_name} -iname "*ic" > m_addresses
	# For each trained model:
	while read -r model
	do
		# Get the model name from its address.
		m_add=$model
		arrM=(${m_add//_/ })
		model_name=${arrM[-1]}
		# The name of the model to be used to compress the dataset.
		m="${file_name}_e${model_name}"
		# Compress the dataset by the model and save the compressed data to the specified address through "monitor" function.
		monitor Tawa-0.7/apps/encode/encode -m "${model}" -i "${data}" -o "zipped/${file_name}_${model_name}_zipped"
		# Get the size of the compressed data.
		c_size=$(stat -c%s "zipped/${file_name}_${model_name}_zipped")
		# Calculate the compression ratio.
		cr=$(echo "scale=6; ${c_size} / ${data_size}" | bc -l)
		# Write the compression statistics of the model to the relevant CSV file.
		printf "${model_name},${cstats},${data_size},${c_size},${cr}\n" >> stats/${file_name}.csv
	done < m_addresses
	# The name of the original Tawa model to be used to compress without training.
	m="${file_name}_eoriginal"
	# Compress the dataset by the original Tawa without training with alphabet size of 256, PPM order set to 5, and the escape probability estimation 
	# method set to D, which saves the compressed data to the specified address, through "monitor" function.
	monitor Tawa-0.7/apps/encode/encode -i "${data}" -o "zipped/${file_name}_original_zipped" -a 256 -e D -O 5
	# Get the size of the compressed data.
    oc_size=$(stat -c%s "zipped/${file_name}_original_zipped")
    # Calculate the compression ratio.
	ocr=$(echo "scale=6; ${oc_size} / ${data_size}" | bc -l)
    # Write the compression statistics of the original model to the relevant CSV file.
	printf "original,${cstats},${data_size},${oc_size},${ocr}\n" >> stats/${file_name}.csv
done < addresses
# Remove the files containing addresses to datasets and models.
rm addresses
rm m_addresses