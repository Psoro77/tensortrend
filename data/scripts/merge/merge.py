import numpy as np
import pandas as pd
from pathlib import Path

current_dir = Path(__file__).parent
stocklist = ["Coca", "Google", "Johnson and Johnson", "Nvidia", "tesla"]
df = pd.DataFrame()
for stockname in stocklist :
    fetch_csv_path = current_dir.parent.parent/ "csv" / f"{stockname}" / "XGBoostdata.csv"
    df_raw = pd.read_csv(fetch_csv_path)
    print( df_raw.shape)
    df = pd.concat([df,  df_raw], ignore_index= True)
print(df.isna().any().any())
path = current_dir.parent.parent / "csv" / "Final_merged"/ "merged_XGB_data.csv"
df.to_csv(path, index=False)



for stockname in stocklist :
    fetch_csv_path = current_dir.parent.parent/ "csv" / f"{stockname}" / "LSTMdata.csv"
    df_raw = pd.read_csv(fetch_csv_path)
    print( df_raw.shape)
    df = pd.concat([df,  df_raw], ignore_index= True)
print(df.isna().any().any())
path = current_dir.parent.parent / "csv" / "Final_merged"/ "merged_LSTM_data.csv"
df.to_csv(path, index=False)