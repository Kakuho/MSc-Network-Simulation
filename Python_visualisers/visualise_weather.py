import seaborn as sns
import matplotlib.pyplot as plt
import pandas as pd

# note this depends on where you have the file rssi_time.txt saved
parent_path = "./../../ns-allinone-3.39/ns-3.39/"
file = "rssi_time.txt"
name = "Plot of RSSI changes Dependent on Time"
path = parent_path + file

def extractNS(value: str) -> str:
    value = value.removeprefix('+')
    value = value.removesuffix("ns")
    return value

def getNS(value: str) -> float:
    ls = value.partition("e")
    value = ls[0]
    return value

def read_dataset() -> pd.DataFrame:
    data = pd.read_csv(path, names = ["seconds", "rssi"], header=None)
    data['seconds'] = data['seconds'].apply(extractNS)
    data['seconds'] = data['seconds'].astype(float)
    return data

def pltRssi(df: pd.DataFrame) -> None:
    plt.plot(df["seconds"], df["rssi"])
    plt.legend(loc="upper left")
    plt.title("RSSI vs Time")
    plt.xlabel("Time (ns)")
    plt.ylabel("RSSI")
    plt.show()

if __name__ == "__main__":
    df = read_dataset()
    pltRssi(df)
