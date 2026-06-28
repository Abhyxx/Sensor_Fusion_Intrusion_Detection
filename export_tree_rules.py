import os
import joblib
from sklearn.tree import export_text

MODEL_FILE = "ids_decision_tree_model.joblib"
FEATURE_FILE = "ids_model_features.joblib"

if not os.path.exists(MODEL_FILE):
    raise FileNotFoundError(
        "ids_decision_tree_model.joblib not found. "
        "Run train_ids_ml.py after saving the Decision Tree model separately."
    )

if not os.path.exists(FEATURE_FILE):
    raise FileNotFoundError(
        "ids_model_features.joblib not found. Run train_ids_ml.py first."
    )

model = joblib.load(MODEL_FILE)
features = joblib.load(FEATURE_FILE)

rules = export_text(model, feature_names=features)

print(rules)

with open("ids_decision_tree_rules.txt", "w") as f:
    f.write(rules)

print("\nSaved tree rules to ids_decision_tree_rules.txt")