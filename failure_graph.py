import matplotlib.pyplot as plt
import numpy as np

total_ops = 10240
num_clients = 8
rw = 50


def get_experiment(protocol):
    if protocol == "SharedReg":
        file_prefix = "sharedresults_results/failed_node_50/throughput_" 
    else:
        file_prefix = "etcd_results/failed_leader_50/throughput_" 
    all_throughput = []
    for i in range(num_clients):
        file_name = file_prefix + str(i) + "_" + str(num_clients) + ".txt"
        file = open(file_name, "r")
        throughput = []
        for l in file.readlines():
            val = int(l)*num_clients
            throughput.append(val)
        throughput = throughput[1:]
        all_throughput.append(throughput)
    
    # need to take average of throughput acrross different clients, but for specific time stamps
    all_throughput_np = np.array(all_throughput)
    mean_all_throughput = np.mean(all_throughput_np, axis=0)
    mean_all_throughput = list(mean_all_throughput)
    x_ax = list(np.arange(0, len(mean_all_throughput)))


    
    return mean_all_throughput, x_ax


plt.xlabel("Timestep (x*50 operations)")
plt.ylabel("Throughput (No. of Operations per second)")

test_proto = ["etcd", "SharedReg"]



for proto in test_proto:
    th_graph,x_axis=get_experiment(proto)
    print(len(th_graph))
    plt.plot(x_axis, th_graph, label="50% Reads, 50% Writes with 8 clients")

plt.legend(loc="upper left")
# plt.show()
plt.savefig('failed_nodes.pdf')
plt.savefig("failed_nodes.png")