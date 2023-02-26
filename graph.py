import matplotlib.pyplot as plt
import numpy as np

total_ops = 10240
th_graph = []
lat_graph = []
def get_experiment(num_clients):
    cumu_time = 0
    for i in range(num_clients):
        file_name = "latency_" + str(i) + "_" + str(num_clients) + ".txt"
        file = open(file_name, "r")
        latency = []
        for l in file.readlines():
            val = int(l)
            latency.append(val)
        total_time = latency[-1]
        cumu_time = max(cumu_time, total_time)
        latency = latency[0:len(latency)-1]
        avg_latency = sum(latency)/len(latency)
        min_latency = min(latency)
        max_latency = max(latency)
        percentile =  np.percentile(latency, 99)
    throughput = (total_ops*1000000)/cumu_time
    th_graph.append(throughput)
    lat_graph.append(avg_latency)
    print(throughput)

experiment = [1,2,4,8,16,32]
for exp in experiment:
    get_experiment(exp)
plt.plot(experiment, th_graph)
plt.savefig("throughput.png")
plt.clf()
plt.plot(experiment, lat_graph)
plt.savefig("latency.png")