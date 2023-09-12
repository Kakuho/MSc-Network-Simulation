import seaborn as sns
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

# the path is dependent on where the files are saved
parent_path = "./../../ns-allinone-3.39/ns-3.39/"
file = "rssi_building"
name = "Plot of RSSI changes Dependent on Time"

def read_dataset_wood() -> pd.DataFrame:
    data = pd.read_csv(parent_path + file + "_wood.txt", names = ["node", "distance", "rssi"], header=None)
    return data

def read_dataset_concrete() -> pd.DataFrame:
    data = pd.read_csv(parent_path+file+"_concrete.txt", names = ["node", "distance", "rssi"], header=None)
    return data

def read_dataset_stone() -> pd.DataFrame:
    data = pd.read_csv(parent_path+file+"_stone.txt", names = ["node", "distance", "rssi"], header=None)
    return data

###############################################################################################

def extract_node0(df: pd.DataFrame) -> pd.DataFrame:
    return df[df["node"] == 0]

def extract_node1(df: pd.DataFrame) -> pd.DataFrame:
    return df[df["node"] == 1]

###############################################################################################

def pltRssi(df: pd.DataFrame) -> None:
    plt.plot(df["distance"], df["rssi"])
    #plt.legend(loc="upper left")
    plt.title("RSSI vs Distance")
    plt.xlabel("Distance")
    plt.ylabel("RSSI")
    #plt.xticks(df['distance'], np.linspace(0, 1))
    plt.savefig(f"RSSI_Distancepng")
    plt.show()

def pltThree(df1, df2, df3) -> None:
    fig, ax = plt.subplots(3, 1, sharex=True, sharey=True)
    l1 = ax[0].plot(df1["distance"], df1["rssi"], color="r", label = "Wood")
    l2 = ax[1].plot(df2["distance"], df2["rssi"], color="g", label = "Concrete")
    l3 = ax[2].plot(df3["distance"], df3["rssi"], color="b", label = "Stone")
    for k in ax:
        k.axvline(x = 0.5, color = 'k', linestyle = "dotted")
    # setting titles of axis and figure
    fig.suptitle("RSSI of recieved Node measured against Distance.")
    for k in ax:
        k.set_ylabel("RSSI")
        k.set_xlabel("Distance")
        k.legend(loc = "upper right")
    #plt.xticks(df['distance'], np.linspace(0, 1))
    plt.savefig(f"RSSI_Distance.png")
    plt.show()

if __name__ == "__main__":
    df = read_dataset_concrete()
    df0 = extract_node0(df)
    print(df0)
    df1 = extract_node1(df)
    pltThree(
            extract_node1(read_dataset_wood()),
            extract_node1(read_dataset_concrete()),
            extract_node1(read_dataset_stone())
            )


