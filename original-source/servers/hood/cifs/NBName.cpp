#include "NBName.h"
#include <cstring>
#include <string>

#include <ctype.h>
#include <stdio.h>



NBName::NBName() {
	m_orig[0] = 0;
	m_net[0] = 0;
	m_num = 0;
}

const char* NBName::orig() {
	return (const char *) m_orig;
}

const char* NBName::net() {
	return (const char *) m_net;
}

int NBName::num() {
	return m_num;
}

void NBName::getNormal(char *name, int &num) {
	strcpy(name, m_orig);
	num = m_num;
}

void NBName::getNBForm(char *nbname) {
	strcpy(nbname, m_net);
}

void NBName::getNBNameNum(int &num) {
	num = m_num;
}
 
bool NBName::operator==(const NBName &rhs) const {
	
	if (m_num != rhs.m_num)
		return false;
	return strcmp(m_orig, rhs.m_orig) ? false : true;

}

bool NBName::operator!=(const NBName &rhs) const {

	return (! operator==(rhs));

}

void NBName::setNormal(const char* readable, int x) {

	strncpy(m_orig,readable, 255);
	strtoupper(m_orig);
	
	m_num = x;
	mangle(m_net, m_orig, (unsigned char) m_num);
	
}

void NBName::setNBNameNum(int num)
{

	printf("NBName::setNBNameNum: old num was 0x%x, new is 0x%x\n",m_num,num);
	m_num = num;
	mangle(m_net, m_orig, (unsigned char) m_num);

}

void NBName::setNBForm(const char* nbname) {

	memcpy(m_net, nbname, 34);
	m_net[33] = 0;
	
	int dumb;
	unmangle(m_net, m_orig, m_num);
}


void NBName::mangle(char *nbname, const char *name, unsigned char num) {

	unsigned char adjusted[16];
	
	strncpy((char *) adjusted, name, 16);
	int name_len = strlen(name);
	for (int i=name_len; i<15; ++i) {
		adjusted[i] = ' ';
	}
	adjusted[15] = num;
	
	nbname[0] = ' ';
	for (int i=1; i<17; ++i) {
		nbname[(2*i) - 1] = (adjusted[i-1] >> 4) + 'A';
		nbname[(2*i)] = (adjusted[i-1] & 0x0F) + 'A';
	}	 
	nbname[33] = 0;  // Assure end to string.  
}

void NBName::unmangle(const char *nbname, char *name, int &num) {

	if ((nbname[0] != ' ') || (nbname[33] != 0)) {
		num = -1;
		return;
	}
	
	unsigned char adjusted[16];
	
	for (int i = 1; i<17; ++i) {
		adjusted[i - 1] = (nbname[(2*i) - 1] - 'A') << 4;
		adjusted[i - 1] |= (nbname[(2*i)] - 'A');
	}

	num = adjusted[15];

	// Turn all ending spaces into nulls
	adjusted[15] = 0;
	for (int i = 14; i > -1; --i) {
		if ((adjusted[i+1] == 0) && (adjusted[i] == ' '))
			adjusted[i] = 0;
	}

//	int adj_len = 0;
//	for (int i = 0; i < 16; ++i) {
//		if (adjusted[i] == ' ')
//			break;
//		++adj_len;
//	}
//	adjusted[adj_len] = 0;

	strcpy(name, (char *) adjusted);
}


// Is there a std c++ lib call for this?
void NBName::strtoupper(char* str) {
int len = strlen(str);
	for(int i = 0; i <= len; i++) {
		str[i] = toupper(str[i]);
	}
}








