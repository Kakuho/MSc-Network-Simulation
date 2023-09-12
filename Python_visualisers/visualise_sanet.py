import pandas as pd
import matplotlib.pyplot as plt

# note this depends on where you stored sanet.output.csv
df = pd.read_csv("./sanet.output.csv")
df = df.fillna(0)

def end_to_end():
    label = "Average End to End"
    plt.plot("SimulationSecond", label, data=df)
    plt.title(label + " delay")
    plt.savefig(f"Average_End_to_End.png")
    plt.show()

def package_delivery():
    label = "Package Delivery Ratio"
    plt.plot("SimulationSecond", label, data=df)
    plt.title(label)
    plt.savefig(f"Package_Delivery_Ratio.png")
    plt.show()

end_to_end()
