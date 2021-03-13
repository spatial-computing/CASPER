﻿CASPER
=========

| Release Info |                                                                     |
|:-------------| :-------------------------------------------------------------------|
| Title        | CASPER for ArcGIS: Evacuation Routing                               |
| Web Page     | https://github.com/spatial-computing/CASPER                         |
| Revision     | REV                                                   |
| Author       | Kaveh Shahabi                                                       |
| Email        | kshahabi@usc.edu                                                    |
| Author Page  | http://www-scf.usc.edu/~kshahabi                                    |
| Description  | A Network Analyst tool to intelligently calculate evacuation routes |

####About
CASPER (Capacity-Aware Shortest Path Evacuation Routing) is a custom Network Analyst tool that uses a state-of-the-art routing algorithm to produce evacuation routes to the nearest safe area for each evacuee or group of evacuees. It is an innovative new ArcGIS tool that can help city officials, public safety and other emergency departments to perform evacuation planning more intelligently and efficiently.

####Requirement
 - [Visual C++ Redistributable Packages for Visual Studio 
2013](http://www.microsoft.com/en-us/download/details.aspx?id=40784) (Install both x86 and x64 versions)
 - ArcGIS Desktop 10.3
 - Network Analyst Extension
 - (Optional) Background Geoprocessing (64-bit)
 - (Optional) ArcGIS Desktop C++ SDK: only if you want to compile the program yourself


####Using the Code
The project can be compiled with VS2013 or later. You need to install ArcGIS Desktop C++ SDK and [Boost libraries](http://www.boost.org/). download the latest version, extract it on your PC, then add its path to VS C++ project include directories ([more info](http://msdn.microsoft.com/en-us/library/73f9s62w.aspx)). You also need a post-commit and a post-checkout git hook to generate an extra header file. Use the following sample:

```bash
#!/bin/bash
echo "post-checkout script: Writing git describe to gitdescribe.h"

echo "#ifndef GIT_DESCRIBE" > src/gitdescribe.h
echo "#define GIT_DESCRIBE \"`git describe`\"" >> src/gitdescribe.h
echo "#endif" >> src/gitdescribe.h
unix2dos src/gitdescribe.h
```

####Installation
In order to install, first unzip the downloaded file.  Next, execute the "install.cmd" script.  This script needs to run as an administrator in Windows Visa and later operating systems.  Make sure any previous CASPER installation is completely uninstalled.  You may find detailed instructions in the user manual.

####Acknowledgement
I would like to thank Esri [Application Prototype Lab (APL)](https://maps.esri.com/), the Esri Network Analyst team, and USC [Spatial Sciences Institute (SSI)](http://spatial.usc.edu/) for their support during the development of this tool. The programing of this tool started when I was a summer intern at Esri in 2011. I worked at APL during that summer and also received tremendous support from the Network Analyst team. After Esri, I continued as a PhD student and research assistant at USC SSI. Thanks to SSI support, I'm still actively developing the CASPER algorithm to improve the future of public safety.

####Copyright / Permission
CASPER is released as a free open-source tool under [Apache 2.0 license](http://www.apache.org/licenses/LICENSE-2.0). For a complete copy take a look at `license.txt`. If you decided to use our tool in your work, we would ask that you cite it as:

K. Shahabi and J. P. Wilson, “CASPER: Intelligent capacity-aware evacuation routing,” Computers, Environment and Urban Systems, vol. 46, pp. 12–24, Apr. 2014

NO WARRANTIES.  The SOFTWARE PRODUCT and any related documentation is provided “as is” without warranty of any kind, either express or implied, including, without limitation, the implied warranties or merchantability, fitness for a particular purpose, or non-infringement. The entire risk arising out of use or performance of the SOFTWARE PRODUCT remains with you.

####Fibonacci Heap
For the heap data structure, the Fibonacci Heap implementation by [Tim Blechmann](http://www.boost.org/doc/libs/1_57_0/doc/html/boost/heap/fibonacci_heap.html) from the [Boost libraries](http://www.boost.org/) has been utilized. The original source code is available under the [Boost License](http://www.boost.org/users/license.html).

####OpenSteer Library
This tool utilizes [OpenSteer](http://opensteer.sourceforge.net/) library to run evacuation simulations. The library is released under [MIT license](http://opensource.org/licenses/mit-license.php).
> Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

> The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
