MpegTsScanner
=============

Scan packets from a MPEG-TS file. One library (tsscanner) provides the basic 
functionality to process through a MPEG-TS file. The other library 
(tsfindfirstprogram) provides functionality to grab information on the first 
program found. This latter functionality is useful for getting information from
a single-program video, such as that recorded from an ATSC/DVB television 
tuner.

Requirements
============

Depends on the LIBDVBPSI library.

Building
========

Execute the following from the project root.

  ./configure
  make

  As admin:

    make install

  To build examples (install must be done first):

    make examples

Example Application
===================

The provided example dumps the information returned using the 
"tsfindfirstprogram" library. Output will be like the following:

General
=======
Program number: 3
       Version: 1
       PCR PID: 3937

Descriptors (Regular)
=====================

0
---
       i_tag: 9
type (local): 4

('unknown' descriptor info)

  data: �n
length: 4

Descriptors (ES)
================

  (New ES section)

     i_type: 2
  type_name: ISO/IEC 13818-2 Video
      i_pid: 3937

  Descriptors (Regular)
  =====================

  0
  ---
         i_tag: 9
  type (local): 4

  ('unknown' descriptor info)

    data: 
  length: 7


  (New ES section)

     i_type: 129
  type_name: User Private
      i_pid: 3938

  Descriptors (Regular)
  =====================

  0
  ---
         i_tag: 10
  type (local): 4

  ('unknown' descriptor info)

    data: eng
  length: 4

  1
  ---
         i_tag: 9
  type (local): 4

  ('unknown' descriptor info)

    data: 
  length: 7

