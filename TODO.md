# TODO

## Line Process
Key assumptions
1. Objects in image do not move from right to left
2. Objects in image do not appear out of the right side of the image
3. Objects in image move from left to right with constant speed
4. Frame size does not change

- Remove ROI
  - Background subtraction the entire frame
  - Find all nested contour in one image
  - Find all nested contour of the next image
  - Match contour between images based on contour position
  - Predict next contour position
  - Record all contour in frame


We will process the 1st image to obtain info i.e., contain target x(s) and store the metrics i.e., (list of masks, rank the masks according to their x position). Then process the 2nd image and compare how many pixels the targets have moved to the right. Determine the whether in the next frame that particular target is still in the frame. At the nth image before the target disappears from the frame we will use the contour of the target as the final result.

Frame 1 (initial frame)
----------------------------------- frame
   x          o         
-----------------------------------

Frame 2 (moving at constant speed)
----------------------------------- frame
           x          o         
-----------------------------------

Frame 3 (new target appears)
----------------------------------- frame
   *                x          o         
-----------------------------------

Frame 4 (practically same as Frame 1)
----------------------------------- frame
             *             x                   
-----------------------------------

Metrics:
- Each target will have a list of x position

Known limitation:
- if not all the frames produce valid contour it will be hard to track