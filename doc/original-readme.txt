
JFS version 2.04   

by Jan E. Mortensen        email:  jemor@inet.uni2.dk
   Lollandsvej 35 3.tv
   DK-2000 Frederiksberg
   Denmark

JFS is a freeware (see the file copyrigh.txt for details) fuzzy system 
distributed as source code in Ansi C. It contains tools for compiling,
running, converting and improving programs written in the language 
Jfl (Jfl-programs are written with a standard text editor). All tools 
are command line tools. 

The only documentation to Jfs is a manual (written in poor english) in
html-format. The manual is distributed in an internal format and converted
to html as part of the installation.

The programs has only been tested under i386 Linux (and Windows 98; the
source code is identical to source1 version 2.04 for Windows).

The installed system takes up around 5 M harddisk space.

Installation:
Run the script 'compile' from the jfs2.04 directory. The script compiles
the jfs-tools (using the compiler gcc) and generates the documentation. 
After installation the jfs2.04 directory should look like:

   jfs2.04/
           bin/              <-- Executables
           doc/              <-- Documentation in internal jhlp-format
               html/         <-- Documentation in html (start in jfs.htm)
           samples/          <-- Sample jfs-programs
           source/           <-- Source code 
               samples/      <-- Sample C programs using source-libraries.

The installation doesn't link the tools to a directory on the users path.
This has to be done manualy, for example:  ln bin/jfc /usr/local/bin,
or (under Linux): install -m 755 bin/jfc /usr/local/bin. Same for the other
tools in the bin directory.

To read the documentation open jfs2.04/doc/html/jfs.htm  in a web browser.

To uninstall Jfs, delete the directory jfs2.04 and all sub-directories. 

Known installation problems:
On some versions of Unix, the documentation is not installed correctly,
i.e., the program 'jhlp' dumps under installation. "Solution:" download
the documentation in html-format from the JFS home page
http://inet.uni2.dk/~jemor/jfs.htm.

Some compile errors with gcc has been reported. Replacing 'gcc' with
 'g++' in the script 'compile' may solve the problems.

