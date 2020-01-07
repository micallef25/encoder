#include <string>
#include <map>

/*
* checks if a chunk exists if it does not increment the counter, store the id for decoder and return -1
* else we have the chunk, send the id to have 32 bit header calculated
* @params 256 bit SHA string
*/
unsigned int find_chunk(unsigned int* sha_int)
{
	static std::map<std::string,int> sha_map;
	static int counter = -1;

	std::string sha = to_string(sha_int)

	if(!sha_map.count(sha)){
		sha_map[sha] = ++counter;
		return -1;
	}
	return sha_map[sha];
}