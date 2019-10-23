## License for most sDNA code: GPL v3

sDNA software for spatial network analysis 
Copyright (C) 2011-2019 Cardiff University

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
    
## License for sDNAUISpec.py

This file is released under the MIT license to ease any licensing issues that may arise if incorporating in plugins to proprietary software. See sDNA.pyt or the sDNA QGIS plugin for examples of its use.
sDNAUISpec specifies a user interface and the means to produce calls to the sDNA command line tools from user input. The command line tools use standard data formats (shapefile or gdb) for interchange and can be used independently of sDNAUISpec, we consider this weak linkage to be permissible under GPL principles (otherwise it would not be possible to run open source tools on a proprietary operating system, or vice versa).

### MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

## Licenses for libraries used in sDNA

shapefile.py - MIT license as above.

muparser has its own permissive license, see muparser source files for details.

GEOS shared library is licensed under GNU LGPL, see GEOS source files for details.

R-portable is licensed under GPL version 3, as a command line tool this is weakly coupled. (It would be possible for the user to use their own installation of R in combination with the sDNA Learn/Predict tools; we provide it in the installer for convenience).