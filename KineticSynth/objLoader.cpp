#include "objLoader.h"

#include <math.h>
#include <stdio.h>
#include <memory>

float _ParseInt(char* Buffer, int* Index, int Length)
{
	int part = 0;
	bool neg = false;

	int ret;

	while (*Index < Length && (Buffer[*Index] > '9' || Buffer[*Index] < '0') && Buffer[*Index] != '-')
		(*Index)++;

	// sign
	if (Buffer[*Index] == '-')
	{
		neg = true;
		(*Index)++;
	}

	// integer part
	while (*Index < Length && !(Buffer[*Index] > '9' || Buffer[*Index] < '0'))
		part = part * 10 + (Buffer[(*Index)++] - '0');

	ret = neg ? (part * -1) : part;
	return ret;
}

float _ParseFloat(char* Buffer, int* Index, int Length)
{
	int part = 0;
	bool neg = false;

	float ret;

	// find start
	while (*Index < Length && (Buffer[*Index] < '0' || Buffer[*Index] > '9') && Buffer[*Index] != '-' && Buffer[*Index] != '.')
		(*Index)++;

	// sign
	if (Buffer[*Index] == '-')
	{
		neg = true;
		(*Index)++;
	}

	// integer part
	while (*Index < Length && !(Buffer[*Index] > '9' || Buffer[*Index] < '0'))
		part = part * 10 + (Buffer[(*Index)++] - '0');

	ret = neg ? (float)(part * -1) : (float)part;

	// float part
	if (*Index < Length && Buffer[*Index] == '.')
	{
		(*Index)++;
		double mul = 1;
		part = 0;

		while (*Index < Length && !(Buffer[*Index] > '9' || Buffer[*Index] < '0'))
		{
			part = part * 10 + (Buffer[*Index] - '0');
			mul *= 10;
			(*Index)++;
		}

		if (neg)
			ret -= (float)part / (float)mul;
		else
			ret += (float)part / (float)mul;

	}

	// scientific part
	if (*Index < Length && (Buffer[*Index] == 'e' || Buffer[*Index] == 'E'))
	{
		(*Index)++;
		neg = (Buffer[*Index] == '-'); *Index++;
		part = 0;
		while (*Index < Length && !(Buffer[*Index] > '9' || Buffer[*Index] < '0'))
		{
			part = part * 10 + (Buffer[(*Index)++] - '0');
		}

		if (neg)
			ret /= (float)pow(10.0, (double)part);
		else
			ret *= (float)pow(10.0, (double)part);
	}

	return ret;
}

struct HashNode
{
	int			vertId;
	HashNode*	next;
};

HashNode** hashMap = new HashNode*[64007];
HashNode* hashNodes = new HashNode[64007];
int hashNodeCount = 0;

vsVertex*	verts = new vsVertex[64000];
vec3*		tempVerts = new vec3[64000];
vec2*		tempUVs = new vec2[64000];
vec3*		tempNormals = new vec3[64000];
uint16_t*	tris = new uint16_t[192000];

int			tempVertCount = 0;
int			tempUVCount = 0;
int			tempNormalCount = 0;
int			triCount = 0;
int			vertCount = 0;

int _GetUniqueVert(char* Buffer, int* Index, int Length)
{
	int vertId = _ParseInt(Buffer, Index, Length); (*Index)++;
	int uvId = _ParseInt(Buffer, Index, Length); (*Index)++;
	int normalId = _ParseInt(Buffer, Index, Length);

	uint32_t hash = 0;
	hash += ((int*)(tempVerts + (vertId - 1)))[0];
	hash += ((int*)(tempVerts + (vertId - 1)))[1];
	hash += ((int*)(tempVerts + (vertId - 1)))[2];

	hash += ((int*)(tempUVs + (uvId - 1)))[0];
	hash += ((int*)(tempUVs + (uvId - 1)))[1];

	hash += ((int*)(tempNormals + (normalId - 1)))[0];
	hash += ((int*)(tempNormals + (normalId - 1)))[1];
	hash += ((int*)(tempNormals + (normalId - 1)))[2];

	hash %= 64007;

	// See if hash exists
	int vertIndex = -1;
	HashNode* next = hashMap[hash];
	while (next != NULL)
	{
		if (verts[next->vertId].pos == tempVerts[vertId - 1] &&
			verts[next->vertId].uv == tempUVs[uvId - 1] &&
			verts[next->vertId].normal == tempNormals[normalId - 1])
		{
			vertIndex = next->vertId;
			break;
		}
		else
		{
			next = next->next;
		}
	}

	if (vertIndex == -1)
	{
		verts[vertCount].pos = tempVerts[vertId - 1];
		verts[vertCount].uv = tempUVs[uvId - 1];
		verts[vertCount].normal = tempNormals[normalId - 1];
		
		HashNode* hashNode = &hashNodes[hashNodeCount++];
		hashNode->next = hashMap[hash];
		hashNode->vertId = vertCount;
		hashMap[hash] = hashNode;

		return vertCount++;
	}
	
	return vertIndex;
}

vsOBJModel CreateOBJ(const char* FileName)
{
	vsOBJModel result = {};

	tempVertCount = 0;
	tempUVCount = 0;
	tempNormalCount = 0;
	triCount = 0;
	vertCount = 0;
	memset(hashMap, 0, sizeof(hashMap) * 64007);
	
	FILE* file = fopen(FileName, "rb");
	fseek(file, 0, SEEK_END);
	int fileLen = ftell(file);
	fseek(file, 0, SEEK_SET);
	char* fileBuffer = new char[fileLen + 1];
	fread(fileBuffer, 1, fileLen, file);
	fileBuffer[fileLen] = 0;

	int idx = 0;
	char c = fileBuffer[idx++];

	while (c != 0)
	{
		if (c == 'v')
		{
			c = fileBuffer[idx++];

			if (c == ' ')
			{
				vec3 attr;
				attr.x = _ParseFloat(fileBuffer, &idx, fileLen); idx++;
				attr.z = _ParseFloat(fileBuffer, &idx, fileLen); idx++;
				attr.y = _ParseFloat(fileBuffer, &idx, fileLen); idx++;
				c = fileBuffer[idx++];

				if (attr.x == 0.0f) attr.x = 0.0f;
				if (attr.y == 0.0f) attr.y = 0.0f;
				if (attr.z == 0.0f) attr.z = 0.0f;
				//attr.x = -attr.x;

				tempVerts[tempVertCount++] = attr;
			}
			else if (c == 't')
			{
				vec2 attr;
				attr.x = _ParseFloat(fileBuffer, &idx, fileLen); idx++;
				attr.y = _ParseFloat(fileBuffer, &idx, fileLen); idx++;
				c = fileBuffer[idx++];

				if (attr.x == 0.0f) attr.x = 0.0f;
				if (attr.y == 0.0f) attr.y = 0.0f;
				
				tempUVs[tempUVCount++] = attr;
			}
			else if (c == 'n')
			{
				vec3 attr;
				attr.x = _ParseFloat(fileBuffer, &idx, fileLen); idx++;
				attr.z = _ParseFloat(fileBuffer, &idx, fileLen); idx++;
				attr.y = _ParseFloat(fileBuffer, &idx, fileLen); idx++;
				c = fileBuffer[idx++];

				if (attr.x == 0.0f) attr.x = 0.0f;
				if (attr.y == 0.0f) attr.y = 0.0f;
				if (attr.z == 0.0f) attr.z = 0.0f;
				
				tempNormals[tempNormalCount++] = attr;
			}
		}
		else if (c == 'f')
		{
			c = fileBuffer[idx++];

			int rootVertId = _GetUniqueVert(fileBuffer, &idx, fileLen);
			int currVertId = _GetUniqueVert(fileBuffer, &idx, fileLen);
			c = fileBuffer[idx++];

			while (c == ' ')
			{
				int nextVertId = currVertId;
				currVertId = _GetUniqueVert(fileBuffer, &idx, fileLen);
				tris[triCount++] = rootVertId;
				tris[triCount++] = currVertId;
				tris[triCount++] = nextVertId;
				c = fileBuffer[idx++];
			}
		}
		else
		{
			while (c != '\n' && c != 0)
				c = fileBuffer[idx++];

			if (c == '\n')
				c = fileBuffer[idx++];
		}
	}

	delete[] fileBuffer;
	fclose(file);

	// TODO: The object just contains pointers to our permanent storage.
	result.verts = verts;
	result.indices = tris;
	result.vertCount = vertCount;
	result.indexCount = triCount;

	//std::cout << "Loaded OBJ " << FileName << " in " << (time * 1000.0) << "ms Verts: " << vertCount << " Tris: " << (triCount / 3) << "\n";

	/*
	// Determine how good the hash table usage is.
	int maxCollisions = 0;
	int emptyBuckets = 0;

	int entryCount[5] = {};

	for (int i = 0; i < 64007; ++i)
	{
		if (hashMap[i] == NULL)
		{
			++emptyBuckets;
			entryCount[0]++;
		}
		else
		{
			int entries = 0;
			HashNode *next = hashMap[i];
			while (next != NULL)
			{
				++entries;
				next = next->next;
			}

			if (entries > maxCollisions)
				maxCollisions = entries;

			entryCount[entries]++;
		}
	}

	std::cout << "Hash stats - Empty Buckets: " << emptyBuckets << " Max Entires: " << maxCollisions << "\n";

	for (int i = 0; i < 5; ++i)
	{
		std::cout << "Entries[" << i << "]: " << entryCount[i] << "\n";
	}
	//*/

	return result;
}