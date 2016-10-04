# Kinect2 Toolbox

### Code for CVPR 2015 Paper
### Understanding Tools: Task-Oriented Object Modeling, Learning and Recognition

Project website: https://xiaozhuchacha.github.io/projects/cvpr15_tool/

Bibtex:
```bash
@InProceedings{zhu2015tool,
    title={Understanding Tools: Task-Oriented Object Modeling, Learning and Recognition},
    author={Zhu, Yixin and Zhao, Yibiao and Zhu, Song-Chun},
    booktitle={IEEE Conference on Computer Vision and Pattern Recognition (CVPR)},
    year={2015}
}
```

# Kinect v2 Toolbox Manual
### Author: Yixin Zhu (yixin.zhu@ucla.edu), Tianmin Shu (tianmin.shu@ucla.edu)

Last revised: October 2016

## Prerequisite
* Kinect SDK [Download Link](http://www.microsoft.com/en-us/download/details.aspx?id=44561).
* Windows 8.1/10.
* Visual Studio 2015 installed or Runtime Libraries installed.
* MATLAB R2014b or newer version for visualization (optional).

## Usage

The toolbox includes two types of code: **Recorders** and **Converters**. **Recorders** dump the Kinect raw data into RGB images, depth images and skeleton data. **Converters** post-process the data collected by Recorders.

**Recorders** include
* ColorRecorder: dump raw RGB images to hard drive
* DepthRecorder: dump raw depth images to hard drive
* ColorDepthRecorder: dump both raw depth and RGB images to hard drive
* SkeletonRecorder: dump skeleton data to hard drive
* ColorDepthSkeletonRecorder: dump RGB, depth and skeleton simultaneously to hard drive. Two versions are provided. The *Mask* version marks the skeleton area in the depth image.

**Converters** include
* RGBDAlign: align RGB images to the depth images
* Depth2PC: convert raw depth images to point cloud .ply files (no color)
* ColorDepth2PC: convert aligned RGB-D images to point cloud .ply files

**We recommend to use the binary files located inside Pipeline forder if you do not need to change anything in the source code.**

## Record Kinect Raw Data with Kinect Studio

You will need Kinect v2 to physically connect to your PC. Please refer to [this page](https://msdn.microsoft.com/en-us/library/Dn782036.aspx) to check whether your machine is compatible with Kinect v2. The most important component is a USB 3.0 port.

Open Kinect Studio, and switch to **Record** tab. Click on **Connect** button, and check all **11 streams**. Once you finish recording, a .xef file will be generated.

![alt text](https://xiaozhuchacha.github.io/projects/cvpr15_tool/toolbox_images/Record.png "Recording GUI setup")

Kinect v2 consumes lots of memory. If you need to record long clips, remember to edit the buffer size before recording. Also, in order to achieve high frame rate when dumping the data into hard drive, it is recommended to have more than 16GB memory for Kinect Studio.

![alt text](https://xiaozhuchacha.github.io/projects/cvpr15_tool/toolbox_images/Buffer.png "Set Kinect Studio buffer for better performance")

## Recorders

In this section, NO Kinect v2 needed to physically connect to PC. Following instructions assume no connection.

* Load the recorded Kinect data (.xef file) that you want to dump using Kinect Studio.
* In PLAY tab, click Connected button.
* Check all 11 streams.

![alt text](https://xiaozhuchacha.github.io/projects/cvpr15_tool/toolbox_images/CheckAll.png "Play settings")

* Hit Play button, and you should be able to see the replayed videos. You can change the stream (RGB, depth, skeleton, etc) in two viewers by clicking on the gear-shape buttons.

![alt text](https://xiaozhuchacha.github.io/projects/cvpr15_tool/toolbox_images/PlayStart.png "Play recorded data")

* Hit Pause button at the beginning of the data that you want to dump.
* Open any **Recorder** mentioned in previous section. You should see a **stilled** skeleton in a pop-up windows. In this manual, we will use **ColorDepthSkeletonRecorder** as the example, since it is the most comprehensive **Recorder** that records RGB, depth and skeleton data simultaneously.

![alt text](https://xiaozhuchacha.github.io/projects/cvpr15_tool/toolbox_images/OpenRecorder.png "Start Recorder")

* Hit Play button to resume playing. The skeleton in the pop-up windows should start to move together with the videos in Kinect Studio. Data starts to dump into the **data**folder.
* Hit Pause button again to stop dumping (before the video runs out), and close the pop-up window.

## Converters

All the **converters** load the data from **data** folder, and write the new data into the same **data** folder.

The inputs and outputs of **converters** are defined as

Converters | Inputs | Outputs | Physically plugin Kinect?
-----------|--------|---------|--------------------------
RGBDAlign | depth2rgb_mapper | raw_depth, raw_rgb | aligned_rgb | No
Depth2PC | raw_depth | point cloud file (.ply) | No
ColorDepth2PC | aligned_rgb, raw_depth | point cloud file (.ply) | Yes

## MATLAB Visualization (optional)

This part of toolbox is designed for visualization only. You can visualize the data easily in other programming languages, e.g. Python, C++, etc.

* Copy all the files from previous **data** folder into the subfolder **data** located in the folder **matlab\_viz**.
* Run **viz\_result.m**. It should show point cloud with human skeleton. The function **showPointCloud** is only supported in MATLAB 2014b or newer version, but can be easily replaced by function **plot3** in older versions.

![alt text](https://xiaozhuchacha.github.io/projects/cvpr15_tool/toolbox_images/MatlabViz.png "MATLAB visualization")
