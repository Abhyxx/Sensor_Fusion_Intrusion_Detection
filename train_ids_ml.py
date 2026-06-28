import os
import glob
import pandas as pd
import numpy as np

from sklearn.model_selection import train_test_split
from sklearn.metrics import classification_report, confusion_matrix, ConfusionMatrixDisplay
from sklearn.tree import DecisionTreeClassifier
from sklearn.ensemble import RandomForestClassifier
from sklearn.linear_model import LogisticRegression
from sklearn.preprocessing import StandardScaler
from sklearn.pipeline import Pipeline
import matplotlib.pyplot as plt
import joblib


DATA_DIR = "raw_dataset"

csv_files = glob.glob(os.path.join(DATA_DIR, "*.csv"))

if len(csv_files) == 0:
    raise FileNotFoundError("No CSV files found inside raw_dataset folder.")

print("Files found:")
for f in csv_files:
    print(" -", os.path.basename(f))

all_data = []

for file in csv_files:
    df = pd.read_csv(file)
    df["source_file"] = os.path.basename(file)
    all_data.append(df)

data = pd.concat(all_data, ignore_index=True)

print("\nCombined shape:", data.shape)
print("\nOriginal labels:")
print(data["label"].value_counts())


# Clean / merge labels

data["label_clean"] = data["label"].replace({
    "human_crossing_one_human": "human_crossing"
})


def make_target_3(label):
    if label.startswith("human_"):
        return "human_intrusion"
    elif label.startswith("object_movement"):
        return "object_movement"
    else:
        return "safe_static_empty"


data["target_3class"] = data["label_clean"].apply(make_target_3)

print("\n3-class target distribution:")
print(data["target_3class"].value_counts())


# Feature engineering

data["distance_clipped"] = data["distance_cm"].clip(upper=500)

data["distance_valid_20_500"] = (
    (data["distance_cm"] >= 20) & (data["distance_cm"] < 500)
).astype(int)

data["close_20_150"] = (
    (data["distance_cm"] >= 20) & (data["distance_cm"] <= 150)
).astype(int)

data["too_close_lt20"] = (data["distance_cm"] < 20).astype(int)

data["abs_speed_cm_s"] = data["speed_cm_s"].abs()
data["abs_distance_change_cm"] = data["distance_change_cm"].abs()


inactive_human_rows = (
    (data["target_3class"] == "human_intrusion") &
    (data["distance_cm"] >= 900) &
    (data["radar_bin"] == 0) &
    (data["abs_distance_change_cm"] <= 2)
)

print("\nInactive human rows removed:", inactive_human_rows.sum())

data = data[~inactive_human_rows].copy()

print("\n3-class target distribution after inactive-row removal:")
print(data["target_3class"].value_counts())


features = [
    "distance_clipped",
    "distance_valid_20_500",
    "close_20_150",
    "too_close_lt20",
    "radar_bin",
    "radar_count",
    "distance_change_cm",
    "speed_cm_s",
    "abs_speed_cm_s",
    "abs_distance_change_cm",
]

X = data[features]
y = data["target_3class"]



# Train-test split


X_train, X_test, y_train, y_test = train_test_split(
    X,
    y,
    test_size=0.25,
    random_state=42,
    stratify=y
)


models = {
    "Decision Tree": DecisionTreeClassifier(
        max_depth=4,
        random_state=42,
        class_weight="balanced"
    ),

    "Random Forest": RandomForestClassifier(
        n_estimators=200,
        max_depth=6,
        random_state=42,
        class_weight="balanced"
    ),

    "Logistic Regression": Pipeline([
        ("scaler", StandardScaler()),
        ("model", LogisticRegression(
            max_iter=2000,
            class_weight="balanced"
        ))
    ])
}


best_model = None
best_score = -1
best_name = None

trained_models = {}

for name, model in models.items():
    print("\n====================================")
    print(name)
    print("====================================")

    model.fit(X_train, y_train)
    trained_models[name] = model
    y_pred = model.predict(X_test)

    print(classification_report(y_test, y_pred))

    score = model.score(X_test, y_test)

    if score > best_score:
        best_score = score
        best_model = model
        best_name = name


print("\nBest model:", best_name)
print("Best test accuracy:", best_score)




# Confusion matrix for best model

y_pred_best = best_model.predict(X_test)

cm = confusion_matrix(y_test, y_pred_best, labels=best_model.classes_)

disp = ConfusionMatrixDisplay(
    confusion_matrix=cm,
    display_labels=best_model.classes_
)

disp.plot(xticks_rotation=30)
plt.title("IDS Confusion Matrix - Best Model")
plt.tight_layout()
plt.savefig("ids_confusion_matrix.png", dpi=300)
plt.show()


# Feature importance for Random Forest / Decision Tree

if best_name in ["Random Forest", "Decision Tree"]:
    importances = pd.Series(
        best_model.feature_importances_,
        index=features
    ).sort_values(ascending=False)

    print("\nFeature importance:")
    print(importances)

    importances.plot(kind="bar")
    plt.title("Feature Importance")
    plt.tight_layout()
    plt.savefig("ids_feature_importance.png", dpi=300)
    plt.show()



# Save model

joblib.dump(best_model, "ids_best_model.joblib")
joblib.dump(trained_models["Decision Tree"], "ids_decision_tree_model.joblib")
joblib.dump(features, "ids_model_features.joblib")

data.to_csv("combined_ids_dataset.csv", index=False)

print("\nSaved:")
print(" - ids_best_model.joblib")
print(" - ids_model_features.joblib")
print(" - combined_ids_dataset.csv")
print(" - ids_confusion_matrix.png")
print(" - ids_feature_importance.png if applicable")

