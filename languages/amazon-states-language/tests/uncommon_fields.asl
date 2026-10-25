{
  "Comment": "Exercises fields the shipped corpus never reaches.",
  "Version": "1.0",
  "TimeoutSeconds": 300,
  "StartAt": "Choose",
  "States": {
    "Choose": {
      "Type": "Choice",
      "Choices": [
        {
          "Variable": "$.value",
          "NumericGreaterThan": 0,
          "Comment": "positive branch",
          "Next": "Work"
        },
        {
          "Or": [
            { "Variable": "$.value", "NumericEquals": 0 },
            { "Variable": "$.value", "IsNull": true }
          ],
          "Next": "Fallback"
        }
      ],
      "Default": "Fallback"
    },
    "Work": {
      "Type": "Map",
      "MaxConcurrency": 2,
      "ItemSelector": {
        "item.$": "$$.Map.Item.Value",
        "index.$": "$$.Map.Item.Index"
      },
      "ItemReader": {
        "Resource": "arn:aws:states:::s3:getObject",
        "ReaderConfig": {
          "InputType": "JSON",
          "MaxItems": 100,
          "MaxItemsPath": "$.limit"
        }
      },
      "ItemProcessor": {
        "ProcessorConfig": {
          "Mode": "INLINE",
          "ExecutionType": "STANDARD"
        },
        "StartAt": "Item",
        "States": {
          "Item": {
            "Type": "Task",
            "Resource": "arn:aws:states:::lambda:invoke",
            "TimeoutSecondsPath": "$.timeout",
            "HeartbeatSeconds": 5,
            "HeartbeatSecondsPath": "$.heartbeat",
            "ResultSelector": {
              "payload.$": "$.Payload"
            },
            "Catch": [
              {
                "ErrorEquals": ["States.ALL"],
                "Comment": "swallow everything",
                "Next": "Fallback"
              }
            ],
            "End": true
          }
        }
      },
      "Next": "Done"
    },
    "Done": {
      "Type": "Pass",
      "ErrorPath": "$.lastError",
      "End": true
    },
    "Fallback": {
      "Type": "Fail",
      "CausePath": "$.failureCause"
    }
  }
}
