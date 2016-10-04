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
* Visual Studio 2012 installed or Runtime Libraries installed.
* MATLAB R2014b or newer version for visualization (optional).

## Usage

The toolbox includes two types of code: **Recorders** and **Converters**. **Recorders** dump the Kinect raw data into RGB images, depth images and skeleton data. **Converters** post-process the data collected by Recorders.

**Recorders** include
* ColorRecorder: dump raw RGB images to hard drive
* DepthRecorder: dump raw depth images to hard drive
* ColorDepthRecorder: dump both raw depth and RGB images to hard drive
* SkeletonRecorder: dump skeleton data to hard drive
* ColorDepthSkeletonRecorder: dump RGB, depth and skeleton simultaneously to hard drive. Two versions are provided. The *Mask* version marks the skeleton area in the depth image.

**Converters** need to run in the following orders, including
* RGBDAlign: align RGB images to the depth images
* Depth2PC: convert raw depth images to point cloud .ply files (no color)
* ColorDepth2PC: convert aligned RGB-D images to point cloud .ply files
