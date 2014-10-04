ArcCASPER
=========

| Release Info |                                                                     |
|:-------------| :-------------------------------------------------------------------|
| Title        | ArcCASPER Evacuation Routing                                        |
| Web Page     | http://esri.com/arccasper                                           |
| Revision     | REV                                                   |
| Author       | Kaveh Shahabi                                                       |
| Email        | kshahabi@usc.edu                                                    |
| Author Page  | http://www-scf.usc.edu/~kshahabi                                    |
| Description  | A Network Analyst tool to intelligently calculate evacuation routes |

####About
ArcCASPER (Capacity-Aware Shortest Path Evacuation Routing) is a custom Network Analyst tool that uses a state-of-the-art routing algorithm to produce evacuation routes to the nearest safe area for each evacuee or group of evacuees. It is an innovative new ArcGIS tool that can help city officials, public safety and other emergency departments to perform evacuation planning more intelligently and efficiently.

####Requirement
 - [Visual C++ Redistributable Packages for Visual Studio 
2013](http://www.microsoft.com/en-us/download/details.aspx?id=40784) (Install both x86 and x64 versions)
 - ArcGIS Desktop 10.2
 - Network Analyst Extension
 - (Optional) Background Geoprocessing (64-bit)

####Using the Code
The project can be compiled with VS2013 or later. You need a post-commit and a post-checkout git hook to generate an extra header file. Use the following sample:

```bash
#!/bin/bash
echo "post-checkout script: Writing git describe to gitdescribe.h"

echo "#ifndef GIT_DESCRIBE" > src/gitdescribe.h
echo "#define GIT_DESCRIBE \"`git describe`\"" >> src/gitdescribe.h
echo "#endif" >> src/gitdescribe.h
unix2dos src/gitdescribe.h
```

####Installation
In order to install, first unzip the downloaded file.  Next, execute the "install.cmd" script.  This script needs to be run as an administrator in Windows Visa and later operating systems.  Make sure any previous ArcCASPER installation is completely uninstalled.  You may find detailed instructions in the user manual.

####Acknowledgement
We would like to thank ESRI APL and Network Analyst teams to support us during the development of this tool.

####Copyright / Permission
ArcCASPER is released as a free closed-source tool.
If you decided to use our tool in your work, we would ask that you cite it as:

K. Shahabi and J. P. Wilson, “CASPER: Intelligent capacity-aware evacuation routing,” Computers, Environment and Urban Systems, vol. 46, pp. 12–24, Apr. 2014

NO WARRANTIES.  The SOFTWARE PRODUCT and any related documentation is provided “as is” without warranty of any kind, either express or implied, including, without limitation, the implied warranties or merchantability, fitness for a particular purpose, or non-infringement. The entire risk arising out of use or performance of the SOFTWARE PRODUCT remains with you.

####OpenSteer Library
This tool utilizes OpenSteer library to run evacuation simulations. The library is released under [MIT licence](http://opensource.org/licenses/mit-license.php).
> Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

> The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
