/*---------------------------  L I C E N S E  --------------------------------*/
/*  This file has been released under a GPL 2 license                         */
/******************************************************************************/
#include <fcio.h>
#include <compression.h>

// ************************************************
// ***			Bitstream Helper Class			***
// ************************************************
class LSBbitstream : public cBitStream
{	public:
		uintf	flags;									// These flags are used internally

		LSBbitstream(byte *data, uintf maxlength);
		byte getBit(uintf index);
		byte getByte(uintf index);
		void insertBit(byte bit);
		void addBit(byte data);
		void addByte(byte data);
		void addStream(cBitStream *src);
		~LSBbitstream(void);
};

#define LSB_bs_allocbuffer	0x00000001

LSBbitstream::~LSBbitstream(void)
{	if (flags & LSB_bs_allocbuffer) fcfree(data);
}

LSBbitstream::LSBbitstream(byte *ptr, uintf mlength)
{	length = 0;
	flags = 0;
	if (ptr)
		data = ptr;
	else
	{	flags |= LSB_bs_allocbuffer;
		data = fcalloc((mlength+7)/8, "BitStream Buffer");
		memfill(data,0,(mlength+7)/8);
	}
	maxlength = mlength;
}

byte LSBbitstream::getBit(uintf index)
{	if (index>=length) return 0;
	byte bit = data[index>>3] & (1<<(7-(index&7)));
	if (bit)
		return 1;
	else
		return 0;
}

void LSBbitstream::insertBit(byte bit)
{	// Insert a bit at the BEGINNING of the bitstream
	uintf i;
	if (length>=maxlength)
	{	msg("Compression Error", buildstr("Bitstream Overflow (maxlength = %i)",maxlength));
	}
	uintf numbytes = (length>>3)+1;
	byte lastcarry = bit;
	for (i=0; i<numbytes; i++)
	{	byte nextcarry = data[i] & 0x01;
		data[i] = data[i]>>1;
		if (lastcarry) data[i] |= 0x80;
		lastcarry = nextcarry;
	}
	length++;
}

void LSBbitstream::addBit(byte bit)
{	// Insert a bit at the END of the bitstream
	if (length>=maxlength)
	{	msg("Compression Error", buildstr("Bitstream Overflow (maxlength = %i)",maxlength));
	}
	if (bit)
		data[length>>3] |= 1<<(7-(length&7));
//	else												// Shouldn't need to insert a 0, because it should already be initialized to 0
//		data[length>>3] &= 255-(1<<(7-(length&7)));
	length++;
}

void LSBbitstream::addByte(byte newbyte)
{	addBit(newbyte & 0x80);
	addBit(newbyte & 0x40);
	addBit(newbyte & 0x20);
	addBit(newbyte & 0x10);
	addBit(newbyte & 0x08);
	addBit(newbyte & 0x04);
	addBit(newbyte & 0x02);
	addBit(newbyte & 0x01);
}

byte LSBbitstream::getByte(uintf index)
{	byte result = getBit(index++)<<7;
	result |= getBit(index++)<<6;
	result |= getBit(index++)<<5;
	result |= getBit(index++)<<4;
	result |= getBit(index++)<<3;
	result |= getBit(index++)<<2;
	result |= getBit(index++)<<1;
	result |= getBit(index++);
	return result;
}

void LSBbitstream::addStream(cBitStream *src)
{	for (uintf i=0; i<src->length; i++)
		addBit(src->getBit(i));
}

cBitStream *newBitStream(byte *data, uintf maxlength)
{	LSBbitstream *b = new LSBbitstream(data, maxlength);
	return b;
}

// ************************************************
// ***				Huffman Compressor			***
// ************************************************
struct sHuffNode
{	uintf		weight;
	uint16		value;		// 0 - 255 = Leaf Node data item, 256 = End Of File,  257 = Branch Node
	cBitStream	*bs;
	intf		Node0index;
	intf		Node1index;
};

struct sHuffTable
{	intf		rootNodeIndex;
	sHuffNode	node[513];
	uintf		numnodes;
	byte		bitStreamBuffer[8192];	// This figure was calculated as the worst case scenario
};

void HUFF_addBitToNode(sHuffNode *node, intf index, byte bit)
{	if (node[index].bs) node[index].bs->insertBit(bit);
	if (node[index].Node0index>=0) HUFF_addBitToNode(node, node[index].Node0index, bit);
	if (node[index].Node1index>=0) HUFF_addBitToNode(node, node[index].Node1index, bit);
}

void HUFF_DeleteTables(void *Table)
{	sHuffTable *table = (sHuffTable *)Table;
	for (uintf i=0; i<table->numnodes; i++)
	{	if (table->node[i].bs) delete table->node[i].bs;
	}
	fcfree(Table);
}

void *HUFF_generatetables(byte *src, uintf srcsize, uintf flags)
{	sHuffNode	*node;				// Finalised nodes are stored here
	intf		nodeindex[257];		// Index to all the current root nodes
	intf		numrootnodes = 257;	// number of current root nodes
	intf		nextfreenode;		// The index of the next available stored node
	intf		i;

	sHuffTable *table = (sHuffTable *)fcalloc(sizeof(sHuffTable), "Huffman Compression Table");
	memfill(table->bitStreamBuffer, 0, sizeof(table->bitStreamBuffer));
	node = table->node;

	memfill(node,0,sizeof(node));

	// Blank all the nodes
	for (i=0; i<257; i++)
	{	node[i].weight = 0;
		node[i].value = (word)i;
		node[i].Node0index = -1;
		node[i].Node1index = -1;
		node[i].bs = NULL;
	}

	// fill the nodes 0-255 with weights from the file
	for (i=0; i<(intf)srcsize; i++)
	{	node[src[i]].weight++;
	}

	// node 256 is the EOF (End Of File) marker
	node[256].weight = 1;

	// remove any unused nodes
	i=0;
	while (i<numrootnodes)
	{	if (node[i].weight==0)
		{	numrootnodes--;
			node[i].weight = node[numrootnodes].weight;
			node[i].value = node[numrootnodes].value;
		}	else
		i++;
	}

	// Create an array of all root nodes
	for (i=0; i<numrootnodes; i++)
	{	nodeindex[i] = i;
		node[i].bs = newBitStream(&table->bitStreamBuffer[i*10],256);
	}

	nextfreenode = numrootnodes;
	while (numrootnodes>1)
	{	// Find the 2 lightest weighted nodes and combine them
		intf lightestindex = nodeindex[0];
		intf nextlightestindex = nodeindex[1];
		if (node[lightestindex].weight > node[nextlightestindex].weight)
		{	intf tmp = lightestindex;
			lightestindex = nextlightestindex;
			nextlightestindex = tmp;
		}

		for (i=2; i<numrootnodes; i++)
		{	if (node[nodeindex[i]].weight < node[lightestindex].weight)
			{	nextlightestindex = lightestindex;
				lightestindex = nodeindex[i];
			}	else
			if (node[nodeindex[i]].weight < node[nextlightestindex].weight)
			{	nextlightestindex = nodeindex[i];
			}
		}

		// Create a new node containing both those nodes
		if (nextfreenode>512) msg("Program bug detected!","Bug Detected in function HUFF_GenerateTables, nextfreenode exceeds 512 (this is impossible)");
		node[nextfreenode].weight = node[lightestindex].weight + node[nextlightestindex].weight;
		node[nextfreenode].Node0index = lightestindex;
		node[nextfreenode].Node1index = nextlightestindex;
		node[nextfreenode].bs = NULL;
		node[nextfreenode].value = 257;				// This is just to enure it's not mistaken for a leaf node
		HUFF_addBitToNode(node, lightestindex,     0);
		HUFF_addBitToNode(node, nextlightestindex, 1);

		// Find the pointer to the lightest node, and tell it to point to the new node instead
		// bool found = false;
		for (i=0; i<numrootnodes; i++)
			if (nodeindex[i] == lightestindex)
			{	nodeindex[i] = nextfreenode++;
				// found = true;
				break;
			}

		// Find the pointer to the next lightest node and delete it
		// found = false;
		for (i=0; i<numrootnodes; i++)
			if (nodeindex[i] == nextlightestindex)
			{	numrootnodes--;
				nodeindex[i] = nodeindex[numrootnodes];
				// found = true;
				break;
			}
	}

	table->numnodes = nextfreenode;
	table->rootNodeIndex = nodeindex[0];
	return table;
}

void HUFF_compressdata(sCompressData *comp)
{	uintf i,EOFnode;

	if (!comp->UCdata)
		msg("Compression Error","Pointer to Uncompressed Source Data was NULL");

	// Obtain nodes from tables
	if (!comp->tables)
		comp->tables = HUFF_generatetables(comp->UCdata, comp->UCsize, 0);
	sHuffTable *table = (sHuffTable *)comp->tables;
	sHuffNode *node = table->node;

	intf nodemap[257];
	for (i=0; i<257; i++)
	{	nodemap[i]=-1;
	}
	// Find the EOFnode, and map the nodes
	for (i=0; i<table->numnodes; i++)
	{	uint16 value = node[i].value;
		if (value<257) nodemap[value]=i;
	}
	if (nodemap[256]<0)
	{	msg("Copression Error","Huffman tables are invalid");
	}
	EOFnode = nodemap[256];

	// Obtain source sizes
	uintf srcsize = comp->UCsize;
	byte  *src = comp->UCdata;

	// Calculate bit size of compressed data
	uintf bitsize = 0;
	for (i=0; i<srcsize; i++)
	{	bitsize += node[nodemap[src[i]]].bs->length;
	}
	bitsize += node[EOFnode].bs->length;

	// Create the bitstream buffer to hold the compressed data
	byte *buf = fcalloc((bitsize+7) / 8,"Compressed Data");
	memfill(buf, 0, (bitsize+7) / 8);

	// Compress the data
	cBitStream *bs = newBitStream(buf, bitsize);
	for (i=0; i<srcsize; i++)
	{	bs->addStream(node[nodemap[src[i]]].bs);
	}

	// Encode the EOF string
	bs->addStream(node[EOFnode].bs);

	comp->bs = bs;
}

void HUFF_decompressdata(sDecompressData *decomp)
{	sHuffTable *table = (sHuffTable *)decomp->tables;
	intf rootnodeindex = table->rootNodeIndex;
	sHuffNode *node = table->node;

	uintf index,bytesWritten,inputbits;
	byte  *buf;
	cBitStream *bs;

	// bool needEOF = false;	// Not Used
	uintf EOFlength = 0;

	if (decomp->UCsize==0)
	{	// We don't have a data size, we must calculate it
		if (decomp->numbits==0) decomp->numbits = 0xffffffff;
		// needEOF = true;
		bs = newBitStream((byte *)(decomp->rawdata), decomp->numbits);
		bs->length = decomp->numbits;
		index = 0;
		while (1)
		{	intf nodeindex = rootnodeindex;
			while (node[nodeindex].Node0index>=0)					// While at a BRANCH node
			{	byte bit = bs->getBit(index++);						// Read the next bit
				if (bit) nodeindex = node[nodeindex].Node1index;	// Branch to path 1
				else	 nodeindex = node[nodeindex].Node0index;	// Branch to path 2
			}
			uint16 value = node[nodeindex].value;					// We reached a leaf node, obtain value
			if (value==256)											// Is this EOF?
			{	EOFlength = node[nodeindex].bs->length;				// save EOF node's length
				break;												// break
			}
			decomp->UCsize++;
			if (index>=decomp->numbits) break;						// Finish if we have read the last bit
		}
		decomp->numbits = index;
	}

	bytesWritten = 0;
	inputbits = decomp->numbits;
	if (!inputbits) inputbits = 0xffffffff;
	buf = fcalloc(decomp->UCsize,"Decompression Buffer");
	memfill(buf,0,decomp->UCsize);
	decomp->UCdata = buf;

	bs = newBitStream((byte *)(decomp->rawdata), inputbits);	// Let's hope the file wasn't expanded past 200%
	bs->length = inputbits;
	index = 0;
	while (bytesWritten<decomp->UCsize)
	{	intf nodeindex = rootnodeindex;
		while (node[nodeindex].Node0index>=0)					// While at a BRANCH node
		{	byte bit = bs->getBit(index++);						// Read the next bit
			if (bit) nodeindex = node[nodeindex].Node1index;	// Branch to path 1
			else	 nodeindex = node[nodeindex].Node0index;	// Branch to path 2
		}
		uint16 value = node[nodeindex].value;					// We reached a leaf node, obtain value
		if (value==256) break;									// Is this EOF?  Break
		*buf++ = (byte)value;
		bytesWritten++;
		if (index>=inputbits) break;
	}
	decomp->numbits = index + EOFlength;
	decomp->UCsize = bytesWritten;
}

void HuffmanShrinkTablesAddNode(cBitStream *bs, sHuffTable *table, intf nodeIndex)
{	sHuffNode *node = &table->node[nodeIndex];
	if (node->Node0index>=0)
	{	// Save a 0 bit to show this is a BRANCH node
		bs->addBit(0);
		HuffmanShrinkTablesAddNode(bs, table, node->Node0index);
		HuffmanShrinkTablesAddNode(bs, table, node->Node1index);
	}	else
	{	// Save a 1 bit to show this is a VALUE node
		bs->addBit(1);

		// Save the actual value
		bs->addByte((byte)node->value);

		// If it's a 0 value, it could be a 0 or 256 (EOF), if so, write a 0 bit for 0, or 1 for 256
		if (node->value==0)
			bs->addBit(0);
		if (node->value==256)
			bs->addBit(1);
	}
}

cBitStream *HUFF_ShrinkTables(void *tables)
{	sHuffTable *table = (sHuffTable *)tables;
	cBitStream *bs = newBitStream(NULL, 4096*8);
	HuffmanShrinkTablesAddNode(bs,table,table->rootNodeIndex);
	return bs;
}

void HUFF_ExpandTablesAddNode(cBitStream *bs, sHuffTable *table, uintf nodeindex)
{	byte bit = bs->getBit(table->rootNodeIndex++);
	sHuffNode *n = &table->node[nodeindex];
	n->weight = 0;
	if (bit==0)
	{	n->value = 257;
		n->bs = NULL;
		n->Node0index = table->numnodes++;
		HUFF_ExpandTablesAddNode(bs, table, n->Node0index);
		n->Node1index = table->numnodes++;
		HUFF_ExpandTablesAddNode(bs, table, n->Node1index);
	}	else
	{	n->value = bs->getByte(table->rootNodeIndex);
		table->rootNodeIndex += 8;
		if (n->value==0)
		{	byte bit = bs->getBit(table->rootNodeIndex++);
			if (bit) n->value = 256;
		}
		n->bs = newBitStream(&table->bitStreamBuffer[nodeindex*10],256);
		n->Node0index = -1;
		n->Node1index = -1;

		// Build a bitstream for this node
		intf lookfor = nodeindex;
		for (intf i=nodeindex-1; i>=0; i--)
		{	if (table->node[i].Node0index==lookfor)
			{	n->bs->insertBit(0);
				lookfor = i;
			}	else
			if (table->node[i].Node1index==lookfor)
			{	n->bs->insertBit(1);
				lookfor = i;
			}
		}
	}
}

void *HUFF_ExpandTables(cBitStream *bs)
{	sHuffTable *table = (sHuffTable *)fcalloc(sizeof(sHuffTable),"Huffman Compression Table");
	memfill(table,0,sizeof(sHuffTable));
	table->rootNodeIndex = 0;
	table->numnodes = 1;
	HUFF_ExpandTablesAddNode(bs, table, 0);
	table->rootNodeIndex = 0;
	return table;
}

/*
void dumptables(void *src1, void *src2)
{	sHuffTable *src = (sHuffTable *)src1;
	sHuffTable *dst = (sHuffTable *)src2;

	textout(5,20,"Table Comparison: %i SRC nodes, %i DST nodes",src->numnodes, dst->numnodes);
	intf y=35;
	uint16 i=0;
	intf srcnode = -1;
	while ((y<screenheight-10) && (i<257))
	{	intf srcnode = -1;
		intf dstnode = -1;
		// search for node [i] in sources
		for (uintf search=0; search<src->numnodes; search++)
		{	if (src->node[search].value==i)
				srcnode = search;
			if (dst->node[search].value==i)
				dstnode = search;
		}
		if (srcnode*dstnode<0)
		{	if (srcnode==-1)
			{	textout(5,y,"SRC does not contain node value %i!",i);
				y+=15;
				i++;
				continue;
			}
			if (dstnode==-1)
			{	textout(5,y,"DST does not contain node value %i!",i);
				y+=15;
				i++;
				continue;
			}
		}
		if (srcnode==-1)
		{	i++;
			continue;
		}
//		textout(5,y,"%i",i);
//		y+=15;
		i++;
	}
}
*/

void HuffmanCompressionPlugin(sCompressor *c)
{	c->code		= "HUFF";
	c->name		= "Huffman";
	c->author	= "Stephen Fraser";
	c->copyright= "";
	c->flags	= compflag_canWriteEOF;

	c->GenerateTables = HUFF_generatetables;	// Consumes just less than 5kb
	c->CompressData	  = HUFF_compressdata;
	c->DecompressData = HUFF_decompressdata;
	c->ShrinkTables	  = HUFF_ShrinkTables;
	c->ExpandTables	  = HUFF_ExpandTables;
	c->DeleteTables	  = HUFF_DeleteTables;
}

// ************************************************
// ***				Compression Control			***
// ************************************************
bool getCompressor(sCompressor *c, char *code)
{	void (*algo)(sCompressor *) = (void (*)(sCompressor *))getPluginHandler(code, PluginType_Compression);
	if (!algo) return false;
	algo(c);
	return true;
}

void initcompressors(void)
{	addGenericPlugin((void *)HuffmanCompressionPlugin, PluginType_Compression, "HUFF");
}
