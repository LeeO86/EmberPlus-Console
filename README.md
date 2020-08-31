# EmberPlus-Console
An Console Application for access [Ember+ Protocol](https://github.com/Lawo/ember-plus) Providers.  
Built for using with Scripts or Automation-Tools like Ansible

## Usage
Use it in any shell:
```bash
$ ./EmberPlus-Console -h
Usage: ./EmberPlus-Console [options] destination "path/:[value]" ...
CLI-Tool to access EmBer+ Provider.

Options:
  -b, --brief              Briefly Output only Results.
  -n, --numbered-path      Use the numbered Ember+ Path instead of the
                           Identifier-Path. The notation is separated with a
                           dot.
                           Example: 1.2.3.1/:'value'
  -q, --quiet              Suppress all Error or Log Messages.
  -t, --timeout <seconds>  Time to wait for changed Parameters. The
                           Connection-Timeout is two times <seconds>. If not set
                           it defaults to 1 second.
  -v, --verbose            Prints the verbose Ember+ Output.
  -V, --version            Displays version information.
  -w, --write              Write the <value> to specified <path>. It has to be
                           entered with the path as one String in the follwoing
                           format: "'path'/:'value'"
                           Example: "RootNode/ChildNode/Parameter/:YourValue"
  -?, -h, --help           Displays help on commandline options.
  --help-all               Displays help including Qt specific options.

Arguments:
  destination              EmBer+ Provider Destination as <IP-Address>:<Port>.
  "path/:[value]" ...      Start-Path of EmBer-Tree to Print or Edit, if not
                           specified we start at root. Ensure that the path is
                           recognised as one string. To be sure encapsulate it
                           with "double qoutes". Multiple "'path'/:'value'"
                           pairs are possible.
```
Currently only Parameters are displayed, EmBER+ MatrixExtension and EmBER+ Functions are not yet supported.  
  
Typical Output:
```bash
$ ./EmberPlus-Console 127.0.0.1 
Starting to connect to ember://127.0.0.1/ ...  connetcted
Walking over Ember+ Tree ...  started 

RootNode/Node1/IntegerParameter/:1
RootNode/Node1/RealParameter/:12.340000
RootNode/Node2/StringParameter/:text
RootNode/Node2/Enumeration/:2
RootNode/Node3/BooleanParameter/:0

Ember+ Action finished.
Thanks for using EmberPlus-Console...
```
  
## Dependencies
This Project uses Qt 5.15  
Make sure that it's v5.15 cause of the use of `Qt::endl`  
For building the libember_slim library yourself you need cmake.

## Build
Frist make shure that the libember_slim library is successfully built for your Architecture and OS.  
To build libember_slim yourself:
```bash
cd lib/libember_slim
mkdir build
cd build
cmake ../CMakeLists.txt
make
```
The make sure that your static linked library is added to the [EmberPlusLib.pri](lib/EmberPlusLib.pri) as IncludePath and Linkeroption.  
  
Now start the build with qmake:
```bash
qmake EmberPlus-Console.pro
make
```

## License
This Project is under [MIT License](LICENSE).  
The libember_slim is under [Boost Software License v1.0](lib/libember_slim/LICENSE_1_0.txt)  

