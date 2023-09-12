import seaborn as sns
import matplotlib.pyplot as plt
import pandas as pd

# note this needs to be changed to the correct parent folder
parent_path = "./../../ns-allinone-3.39/ns-3.39/"
file = "output.txt"
name = "Spatial Distribution"
path = parent_path + file
data = pd.read_csv(path, index_col="node", names = ["node", "x", "y"])

def read_data():
    print(data)

# x is the number of nodes in the simulation.
def plot(x):
    for i in range(0, x):
        plt.plot(
                (data.loc[i])["x"],
                (data.loc[i])["y"],
                label = "node " + str(i)
                )
    plt.legend(loc="upper left")
    plt.title(name)
    plt.xlabel("x")
    plt.ylabel("y")
    plt.savefig(f"{name}png")
    plt.show()

if __name__ == "__main__":
    read_data()
    plot(10)
