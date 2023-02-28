import matplotlib.pyplot as plt
import numpy as np

total_ops = 10240

def get_experiment(num_clients, rw):
    th_graph = []
    lat_graph = []
    cumu_time = 0
    for i in range(num_clients):
        file_name = "sharedresults_results/" + str(rw) + "/latency_" + str(i) + "_" + str(num_clients) + ".txt"
        file = open(file_name, "r")
        latency = []
        for l in file.readlines():
            val = int(l)
            latency.append(val)
        total_time = sum(latency[:-1])
        cumu_time = max(cumu_time, total_time)
        latency = latency[0:len(latency)-1]
        avg_latency = sum(latency)/len(latency)
        min_latency = min(latency)
        max_latency = max(latency)
        percentile =  np.percentile(latency, 99)
    throughput = (total_ops*1000000)/cumu_time
    # th_graph.append(throughput)
    # lat_graph.append(avg_latency/1000000)
    print("## ", num_clients, "     ", cumu_time)
    return throughput, avg_latency/1000000

experiment_nodes = [1,2,4,8,16,32]
experiment_rw = [100,50,0]
plt.xlabel("Throughput (No. of Operations per second)")
plt.ylabel("Avg Latency (seconds)")

for rw in experiment_rw:
    th_graph = []
    lat_graph = []
    for exp in experiment_nodes:
        th, lat = get_experiment(exp, rw)
        th_graph.append(th)
        lat_graph.append(lat)
    plt.plot(th_graph, lat_graph, label=str(rw) + "% Reads, " + str(100-rw) + "% Writes")

plt.legend(loc="upper left")
# plt.show()
plt.savefig('latency_throughput.pdf')
plt.savefig("latency_throughput.png")