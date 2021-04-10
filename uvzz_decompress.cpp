/* uvzz_decompress.cpp

   Placeholder starter code for A3

   B. Bird - 06/23/2020
   Georgia 
*/

#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <algorithm>

const uint32_t MAX_UNIQUE = 0b111111-2; //Actual maximum is two below this value (One entry is reversed for EOB, and the other for switch reading modes)
const uint32_t UNIQUE_BIT = 6; //Number of bits used for MAX_UNIQUE

const uint32_t DELTA_BIT = 4; //How many bits delta uses
const uint32_t DELTA_SWITCH = 0b1111;

uint32_t bits_received = 0;
uint32_t bookmark = 0;
bool delta_mode = false;

bool new_block = true;
uint32_t ref = 0; //What value the delta is using for reference
bool is_switch = false;
bool End = false;

/** toBinaryStr
 * This code turns an uint32_t uint32_to a 'bit' string. (This allows direct manipulation of signal bits via a vector datatype)
 * This code was oringally borrowed from https://stackoverflow.com/questions/22746429/c-decimal-to-binary-converting
 * Creator: Logman from StackOverFlow
 * 
 * Param: uint32_t n, the uint32_teger to be turned uint32_to a 'bit' string
 * Return: string, a string consisting of 1's and 0's, referred to as a 'bit' string within this program.
 */
std::string toBinaryStr(uint32_t n)
{
    std::string r;

    if(n == 0) r = "0";

    while(n != 0) 
    {
        r = (n%2 == 0 ?"0":"1") + r; 
        n/= 2;
    }
    while((uint32_t)r.length() < 8)
    {
        r.insert(0,"0");
    }
    std::reverse(r.begin(),r.end());

    return r;
}

/** Extract Overhead Information
 *  This function takes in the overhead information (at the beginning of a block) and computes the inital mtf list used
 *  by the compressor. 
 *  @param block : A block of input from the file being converted oringally designed to only take in a random portion of 
 *                 information and extract the overhead, however it is currently a string of all the unprocessed data.
 *  @return returns the inital mtf used by the compressor.
 */
std::list<uint32_t> Extract_OverheadInfo(std::string& block)
{
    std::list<uint32_t> mtf = {};
    uint32_t run_type = {};
    bits_received = 0;
    if(!block.empty())
    {
        for(uint32_t i=0; i < (uint32_t)block.length(); i+=2)
        { 
            if(block[i] == '0' && block[i+1] == '0') 
            {
                mtf.emplace_back(0);
                run_type = 0;
                bits_received++;
            }
            else if(block[i] == '0' && block[i+1] == '1')
            {
                mtf.emplace_back(1);
                run_type = 1;
                bits_received++;
            }
            else if(block[i] == '1' && block[i+1] == '0')
            {
                for(uint32_t i=0; i<5 ;i++)
                {
                    mtf.emplace_back(run_type);
                }
                bits_received+= 5; 
            }
            else if(block[i] == '1' && block[i+1] == '1')
            {
                for(uint32_t i=0; i<25 ;i++)
                {
                    mtf.emplace_back(run_type);
                }
                bits_received+= 25;  
            }else{
                std::cerr<<"\nCorrupt File: Overhead Error\n";
                exit(EXIT_FAILURE);
            }


            if(bits_received == 256)
            {
                bookmark = i+1;
                break;
            }
            else if(bits_received > 256)
            {
                std::cerr<<"\nCorrupt File: Overhead Error\n";
                exit(EXIT_FAILURE);
            }
        } 
    }
    return mtf;
}

/** Move to Front
 *  This function preforms the move to front algorthim. It also decompresses the delta compression and
 *  is responsible for sending the uncompressed data to the stream.
 * 
 *  @param mft : A list which the move to front it performed on. This list contains active characters and shuffles around
 *               according to the move to front algorthim
 *  @param compressed_data : A string of all the input from the file being read in. (Oringally designed to only hold a chunk of data.)
 * 
 *  @return mtf : This is returned to conserve the current positioning of the mft. This is done to prevent errors due to mft not syncing up.
 */
std::list<uint32_t> Move_to_Front(std::list<uint32_t>& mtf, std::string* compressed_data)
{
    std::list<uint32_t>::iterator it;
    uint32_t idx = -1;
    uint32_t decompressed = 0;

    while(compressed_data->size() > 0)
    {
        if(delta_mode == false) //Read in a character
        {
            decompressed = stoi(compressed_data->substr(0,UNIQUE_BIT),0,2); 
            bits_received += UNIQUE_BIT;
            compressed_data->erase(0,UNIQUE_BIT);
            if(decompressed == MAX_UNIQUE+2) 
            {
                delta_mode = true;
                is_switch = true;
            }
            else if(decompressed == MAX_UNIQUE+1) //EOB Found
            {
                if(compressed_data->size() > 13)
                {
                    new_block = true;
                }else{
                    End = true;
                }
                break;
            }else{
               ref = decompressed;
            }
        }else{ //In delta mode
            if(compressed_data->compare(0,1,"1") == 0) //Then value is negative
            {
                if(stoi(compressed_data->substr(0,DELTA_BIT),0,2) == DELTA_SWITCH) 
                {
                    delta_mode = false;
                    is_switch = true;
                }else{
                    decompressed = -1* stoi(compressed_data->substr(1,DELTA_BIT-1),0,2);
                }
            }else{
                decompressed = stoi(compressed_data->substr(0,DELTA_BIT),0,2);
            }
            bits_received += DELTA_BIT;
            compressed_data->erase(0,DELTA_BIT);
            decompressed += ref;
        }
        
        if(!is_switch)
        {
            idx = -1;
            for(it = mtf.begin(); it != mtf.end(); it++)
            {
                idx++;
                if(decompressed == idx)
                {
                    std::cout.put(*it);
                    mtf.splice(mtf.begin(),mtf,it);
                    break;
                }
            }
        }
        is_switch = false;
    }

    return mtf;
}

int main(){

    char c;
    std::cin.get(c);

    if((uint32_t)c != (uint32_t)0x1f) //0x1f
    {
        std::cerr<<"\nIncorrect File Type 1\n";
        exit(EXIT_FAILURE);
    }
    std::cin.get(c);

    if((int)c != -106) //0x96
    {
        std::cerr<<"\nIncorrect File Type 2\n";
        exit(EXIT_FAILURE);
    }

    std::string input = {};
    uint32_t bytes_read = 0;
    std::list<uint32_t> overhead = {};
    std::list<uint32_t> mtf = {};

    while(std::cin.get(c)) //&& bytes_read < 5120)
    {
        uint32_t i = (unsigned char)c;
        bytes_read++;

        input += toBinaryStr(i);
    }
    while(!End)
    {
        if(new_block) 
        {
            overhead = Extract_OverheadInfo(input); //EOB Can't be found here.
            
            mtf.clear();
            uint32_t sym = 0;
            for(auto i: overhead)
            {
                if(i == 1) mtf.emplace_back(sym);
                sym++; //May need to convert to char
            } 

            is_switch = false;
            ref = 0;
            input.erase(0,bookmark+1); //Overhead data is no longer needed.
        }
        new_block = false;
        bits_received = 0;
        mtf = Move_to_Front(mtf, &input);

        //if(bytes_read < 5120) break;
        bytes_read = 0;
    }
    return 0;
}