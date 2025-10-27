# Remove contrast enhancement feature

- Scope: Remove contrast enhancement config, parsing, and application
- Files updated:
  - include/config.json: deleted image_processing.contrast_enhancement block
  - include/image_processing/image_processing.h: removed contrast fields from ProcessingConfig; removed mats.enhanced
  - src/image_processing/image_processing_utils.cpp: removed contrast defaults/parsing, removed enhancement usage and debug strings, simplified updateBackground functions, adjusted ProcessingConfig constructors
  - src/image_processing/image_processing_core.cpp: removed enhancement path and mats.enhanced usage
  - src/image_processing/image_processing_threads.cpp: removed UI and background enhancement logic
- Rationale: Feature not needed; simplifies pipeline
- Notes: Ensure any external batch configs do not include contrast_enhancement; constructors adjusted accordingly.
