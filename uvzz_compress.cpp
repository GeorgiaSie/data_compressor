/* uvzz_compress.cpp

   Placeholder starter code for A3

   B. Bird - 06/23/2020
   Georgia
*/

#include <iostream>
#include "output_stream.hpp"
#include <map>
#include <vector>
#include <list>

OutputBitStream stream {std::cout};


const uint32_t MAX_UNIQUE = 0b111111-2; //Actual maximum is two below this value (One entry is reversed for EOB, and the other for switch reading modes)
const uint32_t UNIQUE_BIT = 6; //Number of bits used for MAX_UNIQUE

const uint32_t DELTA = 6; //Range of deviation the delta allows. Uses 4 bits, MSB represents +/-.
const uint32_t DELTA_BIT = 4; //How many bits delta uses
const uint32_t DELTA_SWITCH = 0b1111;
//Note: one value is reserved for switching reading formats (delta format and symbol/index values format)

uint32_t total_bits = 0;

/** Send Signed Bits
 *  Sends signed bits values to the stream. Num_bits specifies how many bits are to represent the value
 *  @param value : The decimal representation of the number being sent.
 *  @param num_bits : The number of bits value should be presented with. (Typically the value of UNIQUE_BIT)
 */
void SendSignedBits(int value, uint32_t num_bits)
{
    std::list<uint32_t> code {};
    total_bits += num_bits;   

    if(value < 0) {
        stream.push_bit((uint32_t)1); 
    }else{
        stream.push_bit((uint32_t)0); 
    }

    num_bits--;

    value = abs(value);
    while (value > 0) 
    {  
        code.emplace_front(value%2);
        value = value / 2;  
        num_bits--;
    }
    stream.push_bits((uint32_t)0,num_bits); 

    for(auto i:code) 
    {
        stream.push_bit((uint32_t)i);
    }  
} 
/** Send Unsigned bits
 *  Sends signed bits values to the stream. Num_bits specifies how many bits are to represent the value
 *  @param value : The decimal representation of the number being sent.
 *  @param num_bits : The number of bits value should be presented with. (Typically the value of UNIQUE_BIT)
 */
void SendUnsignedBits(int value, uint32_t num_bits)
{
    std::list<uint32_t> code {};

    total_bits += num_bits;   
    value = abs(value);
    while (value > 0) 
    {  
        code.emplace_front(value%2);
        value = value / 2;  
        num_bits--;
    }
    stream.push_bits(0,(uint32_t)num_bits);
    for(auto i:code) 
    {
        stream.push_bit((uint32_t)i); 
    }
} 

/** Delta Compression
 *  This function takes care of compressing the data using delta compression. This function is also responsible
 *  for adding the "switches" to the data (This is how the decompressor knows to switch reading modes) and putting the compressed data uint32_to the stream.
 *  @param compressed_data : This is the data to be compressed, compressed_data contains the indices produced by move to front.
 */
void DeltaCompression(std::vector<uint32_t> compressed_data) 
{
    uint32_t ref = compressed_data[0];
    uint32_t mode = 0; //Indicates whether we are in character(0) or delta(1) mode
    SendUnsignedBits(compressed_data[0], UNIQUE_BIT);

    for(uint32_t i=1; i < (uint32_t)compressed_data.size(); i++)
    {       
        int change = compressed_data[i] - ref;
        if(abs(change) <= (int)DELTA) //If the difference to reference is less then uint32_terval then send delta
        {
            if(mode == 0) //If in character mode  
            {
                stream.push_bits(MAX_UNIQUE+2, UNIQUE_BIT); 
                mode = 1;
                total_bits += UNIQUE_BIT;   
            }
            SendSignedBits(change, DELTA_BIT);


        }
        else // Send character
        {
            if(mode == 1)
            {
                //Change to character mode
                stream.push_bits(DELTA_SWITCH, DELTA_BIT); 
                mode = 0;
                total_bits += DELTA_BIT;   
            }
            SendUnsignedBits(compressed_data[i], UNIQUE_BIT);
            ref = compressed_data[i];
        }
    }
    if(mode == 1) 
    {
        stream.push_bits(DELTA_SWITCH, DELTA_BIT); //For EOB
    }
}

/** Send Overhead 
 *  This function sends the overhead data to the stream.
 *  @param overhead : A list of one's and zero's indiciating which characters are used in the current block
 *                    number of one's is dependant on MAX_UNIQUE
 */
void SendOverhead(std::vector<uint32_t> overhead)
{
    uint32_t count = 0; //Counts groupings of consecutive 1's or 0's
    uint32_t i = 0;
    uint32_t val = 0;
    while (i < (uint32_t)overhead.size())
    {
        val = ((uint32_t)overhead[i] == 0) ? 0:1;
        if(val == 0)
        {
            stream.push_bit(0);
            stream.push_bit(0);
        }else{
            stream.push_bit(0);
            stream.push_bit(1);
        }
        total_bits += 2;
        i++;
        while(i > overhead.size() && (uint32_t)overhead[i] == val) 
        {
            count++;
            i++;
            if(i+1 > overhead.size())
            {
                break;
            }
        }
        while(count >= 25)
        {
            stream.push_bit(1);
            stream.push_bit(1);   

            count -= 25;   
            total_bits += 2;      
        }
        while(count >= 5)
        {
            stream.push_bit(1);
            stream.push_bit(0);   

            count -= 5;   
            total_bits += 2;  
        }
        while(count >= 1)
        {
            if(val == 1)
            {
                stream.push_bit(0);
                stream.push_bit(1);   

                count --;   
                total_bits += 2;                  
            }else{
                stream.push_bit(0);
                stream.push_bit(0);

                count --;   
                total_bits += 2;  
            }   
        }
    }
}

/** Move to Front
 *  Performs the move to front algorthim using the characters present in the block. mtf will never exceed MAX_UNIQUE
 *  @param block : The current block of information to be compressed
 *  @param overhead : A list containing the number of active characters within the block (contain MAX_UNIQUE active elements)
 * 
 * @return results : Returns a vector of the indices produced by move to front algorthim.
 */
std::vector<uint32_t> Move_to_Front(std::list<uint32_t> block, std::vector<uint32_t> overhead)
{
    std::list<uint32_t> mtf = {};
    std::vector<uint32_t> results = {};
    std::list<uint32_t>::iterator it;
    uint32_t idx = -1;

    for(uint32_t i=0; i < (uint32_t)overhead.size(); i++)
    {
        if(overhead[i] == 1)
        {
            mtf.emplace_back(i);
        }
    }

    for(auto sym: block)
    {
        idx = -1;
        for(it = mtf.begin(); it != mtf.end(); it++)
        {
            idx++;
            if(sym == *it)
            {
                results.emplace_back(idx);
                mtf.splice(mtf.begin(),mtf,it);
                break;
            }
        }
    }
    return results;
}

int main()
{
    uint32_t unique = 0; //Number of unique characters collected
    std::vector<uint32_t> overhead(256,0);
    std::list<uint32_t> block {};

    char c;
    if (!std::cin.get(c))
    {
        //Empty input   
    }else{
        stream.push_bytes( 0x1f, 0x96); //Magic Number

        block.emplace_back((unsigned char)c);
        overhead[(unsigned char)c] = 1;
        unique++;

        while(1)
        {
            
            while(unique < MAX_UNIQUE && std::cin.get(c))
            {
                block.emplace_back((unsigned char)c);
                if(overhead[(unsigned char)c] != 1) 
                {
                    unique++;
                }

                overhead[(unsigned char)c] = 1;
            }

            SendOverhead(overhead);
            std::vector<uint32_t> compressed_data = Move_to_Front(block,overhead);
            DeltaCompression(compressed_data);

            //Push EOB symbol (Always 11...10, number of 1's depends on UNIQUE_BIT) 
            SendUnsignedBits(MAX_UNIQUE+1,UNIQUE_BIT);
            if(unique < MAX_UNIQUE)
            {
                uint32_t padding = 8 - (total_bits % 8);
                stream.push_bits(0,padding);
            }
            if(unique < MAX_UNIQUE) break; //We have read in the whole file.

            //Reset the values to 0.
            for(uint32_t i=0; i<(uint32_t)overhead.size(); i++) 
            {
                overhead[i] = 0;
            }
            unique = 0;
            block.clear();
        }

        stream.flush_to_byte();
    }
    return 0;
}