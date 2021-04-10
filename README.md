# data_compressor
This repository contains a school project for a data compression course. The repository contains a my own compression scheme. The compressor and decompressor are both included.

Author: Georgia


 # Basic Run Through Per Block:

    OverHead: A map -> RLEed 
    Main Data: Move-to-Front Transform (up to 6 bits/31 unique symbols) -> Delta Compression (Variance/bit amount to be determined) -> (Not implemented) RLE the data

Map/Overhead vector: each key represents ascii symbols (0-255), a one indicates an ascii character at that position is active/present within the block, a zero denotes the character is not used within the block. 
This map then get's RLEed. 0's may be the only values the RLE impacts
A map -> RLEed 

Move-to-Front Transform: The initial move to front vector is in ascending ascii order. This is because the map sends the list of active characters in ascending order.

    _____________
    Block Formant:
    -------------

  +========================================================+============================================================+
  | (RLE applied) Overhead (255 array of one's and zero's) | Compressed File Data (Has Delta Compression applied to it) | Another Block following the same format.
  +========================================================+============================================================+

  -Overhead will always consist a total of MAX_UNIQUE one's (Currently set to 61). 
  -Compressed file data consists of the these MAX_UNIQUE (61) characters. The block ends once a new character not apart of the 61 characters is encountered.

  ___________
     Misc.:
  -----------
  -There are two "switch" codes (0b111111 and 0b1111), this allows the decompressor to know what "reading mode" to be in.
    For example if the decompressor sees 0b111111 it knows the next code is in 4 bits not 6 bits.
  -0b111110 Is used as an EOB indicator this allows the decompressor to know it's reading in overhead data next and will switch to a 2 bit
   "reading mode". 



 # In-depth Implementation and Decision making:

   ___________
    OverHead:
   -----------

General:
    -I decided to clear the overhead vector at the end of a block's transmission rather than during the output stage of overhead, since each block is likely to contain more than 256 characters. (the size of overhead)
    It's more efficient to clear the overhead than to store the compressed data and then send the compressed data afterwards (I would be essentially running through compressed data twice), as a trade off for clearing overhead while it's being output to stream.

RLE:
    Uses a method similar to DEFLATE's Block Type 2 RLE for CL codes.
    - overhead (the vector) only contains 1's and 0's. This allows me to use other values to indicate number of runs. 
        I decided to stick with a limit of 2 bits, mainly for simplicity (If I have time I may revisit this and figure out a better quantity of bits)
        00 represents 0,
        01 represents 1,
        10 represents a run of 5,
        11 represents a run of 25.

Why have Overhead Data:
	-I decided to include overhead data because it allowed me to store 8 bit values into just 6 bits. This is possible because each ascii character is represented as an index in the Move-to-Front algorithm. I have the ability to choose how long or short the move-to-front vector is meaning I don't have to use all 256 ascii characters at once. Using overhead data allowed me to leverage this effectively.
	- A shorter move to front vector also means less chance of their being a greater deviation between values, thus increasing the chances of delta compression being preformed on the values.
	-Since the overhead contains only one's and zero's (and considering some files are only text files) it's highly susceptible to RLE and should not take up to much space.

   ___________
    Main Data:
   -----------



Move-to-Front Transform: 
-----------------------


        Move-to-Front's Operations:
            Move-to-Front will collect up to 62 unique characters, the block ends once 62 unique characters are collected.
            -The overhead map is updated as unique characters come in
            At this point the move-to-front will run as normal. (Indices are changed as more common characters come up)
            The block is then sent to the Delta Compression stage.
    Why 6 bits or 62 unique characters was chosen for move-to-front's array limit:
        -6 bits was chosen as 62 unique characters can be stored up. (If we split the ascii table into "common" ascii (symbols 0-127, typically characters used in english literature)
        and "higher" ascii (symbols 128-255), we find about 49% of characters are accounted for. This of course excludes the re-occurrences we will encounter, which will further benefit compression.)
        -Using 6 bits also allows us to theoretically save 2 bits for every 8 bits character.
        -0b111111/63 is reserved for the EOB
        -A list was chosen as insertion and deletions are quicker. More common characters will be at the front of list. 
        -The limit was decided upon to help the O(n^2) complexity of move-to-front. Having serval smaller run throughs is better than having few large run throughs
        of data, as it results in less movement operations in total.




Delta Compression:
------------------


    Delta Compression was designed to be the next stage after Move-to-Transform as Move-to-Transform will create less variance between common characters. 
    Making it idea for delta compression.

    For my delta compression I decided to take what I call the "lots of variance" approach versus the "scaling approach".
    -The scaling approach uses the most previously encountered number as it's reference for the delta value
    -The "lots of variance" approach uses a reference number for incoming values, each value that meets the required ranged is based on the reference number not the number
    previous to it.

    A 4 bit number is used to represent the delta values. The most significant bit is reserved for the sign of the value. 
    -This allows there to be deviation of 7. 
    With the move to front algorithm this will hopefully provide good compression as common values have less variance in them due to being localized to the front.
    It was also decided to include some overhead since having less indices in the array allows more chances for delta compression to occur. The overhead could effectively be condensed using RLE.

    (Not Implemented) Compression could be further improved by choosing to send out a list of characters instead of deltas.
                            For instance let's say move to front produces "0 1 2 20". This gets encoded as:
                            000000  111111  0001  0010  1111  010100     This uses 30 bits
                              0     switch   +1    +2  switch  20
                            
                            Now consider what would happened if the delta wasn't applied:
                            000000 000001 000010 010100                  This uses 24 bits
                     
                     In this instance not using the delta compression is more beneficial and would save more bits.
                     In this situation the compressor should choose to opt out of delta compression and just use the 6 bit representation. 
                     A run of 6 or more is optimal.





 # Citations: 


output_stream.hpp, written by Bill Bird
RLE method from DEFLATE's Block 2 type (Used in overhead)

