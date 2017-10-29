/**************************************************************************/
/*!
    @file     cie_PN532.cpp
    @author   Developers Italia
    @license  BSD (see License)

	
	This is a library for the PN532 NFC/RFID breakout boards
	This library works with the Adafruit NFC breakout 
	----> https://www.adafruit.com/products/364
	
	Check out the links above for our tutorials and wiring diagrams 
	These chips use SPI or I2C to communicate.
	This library uses SPI to communicate.

	@section  HISTORY

	v1.0  - Binary reading of unencrypted elementary files
*/
/**************************************************************************/
#include "cie_PN532.h"

#define PN532DEBUGPRINT Serial

/**************************************************************************/
/*!
  @brief Create with the typical breakout wiring, as described by Adafruit: https://learn.adafruit.com/adafruit-pn532-rfid-nfc/breakout-wiring
*/
/**************************************************************************/
cie_PN532::cie_PN532 () : cie_PN532(2, 5, 3, 4) { 
}

/**************************************************************************/
/*!
  @brief Create with a customized wiring
*/
/**************************************************************************/
cie_PN532::cie_PN532 (byte clk, byte miso, byte mosi, byte ss)
{
  _nfc = new Adafruit_PN532(clk, miso, mosi, ss);
  initFields();
}

/**************************************************************************/
/*!
  @brief Create with a custom instance of the Adafruit_PN532 class
*/
/**************************************************************************/
cie_PN532::cie_PN532 (Adafruit_PN532* nfc)
{
  _nfc = nfc;
  initFields();
}

/**************************************************************************/
/*!
  @brief Set default values to some private fields
*/
/**************************************************************************/
void cie_PN532::initFields() {
  _currentDedicatedFile = NULL_DF;
  _currentElementaryFile = NULL_EF;
  _berReader = new cie_BerReader(this);
  _atrReader = new cie_AtrReader(this);
}


/**************************************************************************/
/*!
  @brief Initialize the PN532 board
*/
/**************************************************************************/
void cie_PN532::begin() {
  _nfc->begin();
  uint32_t versiondata = _nfc->getFirmwareVersion();
  if (! versiondata) {
    PN532DEBUGPRINT.print(F("Didn't find PN53x board"));
    while (1); // halt
  }
  // Got ok data, print it out!
  PN532DEBUGPRINT.print(F("Found chip PN5")); PN532DEBUGPRINT.println((versiondata>>24) & 0b11111111, HEX); 
  PN532DEBUGPRINT.print(F("Firmware ver. ")); PN532DEBUGPRINT.print((versiondata>>16) & 0b11111111, DEC); 
  PN532DEBUGPRINT.print('.'); PN532DEBUGPRINT.println((versiondata>>8) & 0b11111111, DEC);
  _nfc->SAMConfig();

  PN532DEBUGPRINT.println(F("PN53x initialized, waiting for a CIE card..."));
}


/**************************************************************************/
/*!
  @brief  Dumps all values present in the CIE. Call this in your loop
*/
/**************************************************************************/
bool cie_PN532::detectCard() {
  bool success = _nfc->inListPassiveTarget();
  if (success) {
    _currentDedicatedFile = NULL_DF;
    _currentDedicatedFile = NULL_EF;
  }
  return success;
}


/**************************************************************************/
/*!
    @brief  Dumps all values present in the CIE
*/
/**************************************************************************/
void cie_PN532::printHex(byte* buffer, word length) {
  _nfc->PrintHex(buffer, length);
}

/**************************************************************************/
/*!
  @brief  Reads the binary content the EF_DH elementary file

  @param  contentBuffer The pointer to data containing the contents of the file
  @param  contentLength The lenght of the file
	
  @returns  A boolean value indicating whether the operation succeeded or not
*/
/**************************************************************************/
bool cie_PN532::read_EF_DH(byte* contentBuffer, word* contentLength) {
  //cie_EFPath filePath = { ROOT_MF, SELECT_BY_SFI, 0x1B }; //efid 0xD004
  cie_EFPath filePath = { ROOT_MF, SELECT_BY_EFID, 0xD004 }; //efid 0xD004
  return readElementaryFile(filePath, contentBuffer, contentLength, AUTODETECT_BER_LENGTH);
}


/**************************************************************************/
/*!
  @brief  Reads the binary content of the EF_ATR elementary file

  @param  contentBuffer The pointer to data containing the contents of the file
  @param  contentLength The lenght of the file

  @returns  A boolean value indicating whether the operation succeeded or not
*/
/**************************************************************************/
bool cie_PN532::read_EF_ATR(byte* contentBuffer, word* contentLength) {
  cie_EFPath filePath = { ROOT_MF, SELECT_BY_SFI, 0x1D }; //efid 0x2F01
  return readElementaryFile(filePath, contentBuffer, contentLength, AUTODETECT_ATR_LENGTH);
}


/**************************************************************************/
/*!
  @brief  Reads the binary content of the SN_ICC elementary file

  @param  contentBuffer The pointer to data containing the contents of the file
  @param  contentLength The lenght of the file

  @returns  A boolean value indicating whether the operation succeeded or not
*/
/**************************************************************************/
bool cie_PN532::read_EF_SN_ICC(byte* contentBuffer, word* contentLength) {
  cie_EFPath filePath = { ROOT_MF, SELECT_BY_EFID, 0xD003 }; //What's the sfi for this file?
  *contentLength = clamp(*contentLength, EF_SN_ICC_LENGTH);
  return readElementaryFile(filePath, contentBuffer, contentLength, FIXED_LENGTH);
}


/**************************************************************************/
/*!
  @brief  Reads the binary content of the EF_ID_Servizi elementary file

  @param  contentBuffer The pointer to data containing the contents of the file
  @param  contentLength The lenght of the file

  @returns  A boolean value indicating whether the operation succeeded or not
*/
/**************************************************************************/
bool cie_PN532::read_EF_ID_Servizi(byte* contentBuffer, word* contentLength) {
  cie_EFPath filePath = { CIE_DF, SELECT_BY_SFI, 0x01 }; //efid 0x1001
  *contentLength = clamp(*contentLength, EF_ID_SERVIZI_LENGTH);
  return readElementaryFile(filePath, contentBuffer, contentLength, FIXED_LENGTH);
}


/**************************************************************************/
/*!
  @brief  Reads the binary content of the EF.Int.Kpub elementary file

  @param  key The pointer to a cie_Key object which will be populated with the modulo and exponent values

  @returns  A boolean value indicating whether the operation succeeded or not
*/
/**************************************************************************/
bool cie_PN532::read_EF_Int_Kpub(cie_Key* key) {
  cie_EFPath filePath = { CIE_DF, SELECT_BY_SFI, 0x04 }; //efid 0x1004
  return readKey(filePath, key);
}


/**************************************************************************/
/*!
  @brief  Reads the binary content of the EF.Servizi_Int.Kpub elementary file

  @param  key The pointer to a cie_Key object which will be populated with the modulo and exponent values

  @returns  A boolean value indicating whether the operation succeeded or not
*/
/**************************************************************************/
bool cie_PN532::read_EF_Servizi_Int_Kpub(cie_Key* key) {
  cie_EFPath filePath = { CIE_DF, SELECT_BY_SFI, 0x05 }; //efid 0x1005
  return readKey(filePath, key);
}

/**************************************************************************/
/*!
  @brief  Performs the challenge/response algorithm with the SDO.Int_Servizi.Kpub to check where this is a valid or cloned card
	
  @returns  A boolean value indicating whether the card is valid or not
*/
/**************************************************************************/
bool cie_PN532::isCardValid() {
  cie_Key key;
  byte responseLength = 20 + STATUS_WORD_LENGTH;
  byte* response = new byte[responseLength];
  bool success = true;
  //Steps of the algorithm
  //1. Read the Servizi_Int.Kpub public key
  //2. Select the Int_Servizi.Kpriv private key
  //3. Perform device authentication to establish a secure messaging context
  //4. Send the Internal Authenticate APDU command with a RND.IFD of 8 bytes
  //5. Verify the signature is correct
  if (!read_EF_Servizi_Int_Kpub(&key)
    || !select_SDO_Servizi_Int_Kpriv()
    || !establishSecureMessaging()
    || !internalAuthenticate_PkDhScheme(response, &responseLength)
  ) {
    success = false;
  }
  //TODO: free resources in a Dispose() method of the cie_Key class
  delete [] key.modulo;
  delete [] key.exponent;
  delete [] response;

  return success;
}


/**************************************************************************/
/*!
  @brief  Print the binary content of the EF_SOD elementary file

  @param  contentBuffer The pointer to data containing the contents of the file
  @param  contentLength The lenght of the file
	
  @returns  A boolean value indicating whether the operation succeeded or not
*/
/**************************************************************************/
bool cie_PN532::print_EF_SOD(word* contentLength) {
  cie_EFPath filePath = { CIE_DF, SELECT_BY_SFI, 0x06 }; //efid 0x1006
  if (!determineLength(filePath, contentLength, AUTODETECT_BER_LENGTH)) {
        return false;
  }

  word offset = READ_FROM_START;
  while (offset < *contentLength) {
    word contentPageLength = clamp(*contentLength-offset, PAGE_LENGTH);
    byte* pageBuffer = new byte[contentPageLength];
    bool success = readBinaryContent(filePath, pageBuffer, offset, contentPageLength);
    if (success) {
      _nfc->PrintHex(pageBuffer, contentPageLength);
    }
    delete [] pageBuffer;

    offset += contentPageLength;
    if (!success) {
      return false;
    }
  }
  return true;
}


/**************************************************************************/
/*!
  @brief  Parses the BER content of the EF_SOD file

  @param  callback A function which will be invoked when a new triple has been read
	
  @returns  A boolean value indicating whether the operation succeeded or not
*/
/**************************************************************************/
bool cie_PN532::parse_EF_SOD(cieBerTripleCallbackFunc callback) {
  word payloadLength;
  cie_EFPath filePath = { CIE_DF, SELECT_BY_SFI, 0x06 }; //efid 0x1006
  return _berReader->readTriples(filePath, callback, &payloadLength, 30);
}


/**************************************************************************/
/*!
  @brief  Selects the SDO.Servizi_Int.Kpriv private key for internal authentication
  
  @returns  A boolean value indicating whether the operation succeeded or not
*/
/**************************************************************************/
bool cie_PN532::select_SDO_Servizi_Int_Kpriv() {
  cie_EFPath filePath = { CIE_DF, SELECT_BY_SDOID, 0x03 };
  return ensureSdoIsSelected(filePath);
}


/**************************************************************************/
/*!
  @brief  Sends an APDU command which is not returning any data to the CIE via the PN532 terminal and checks whether the response has a valid status word
  
  @param  command A pointer to the APDU command bytes
  @param  commandLength Length of the command

  @returns  A boolean value indicating whether the operation succeeded or not
*/
/**************************************************************************/
bool cie_PN532::sendCommand(byte* command, const byte commandLength) {
  byte responseLength = STATUS_WORD_LENGTH;
  byte* responseBuffer = new byte[responseLength];
  bool success = sendCommand(command, commandLength, responseBuffer, &responseLength);
  delete [] responseBuffer;
  return success;
}


/**************************************************************************/
/*!
  @brief  Sends an APDU command to the CIE via the PN532 terminal and checks whether the response has a valid status word
  
  @param  command A pointer to the APDU command bytes
  @param  commandLength Length of the command
  @param  response A pointer to the buffer which will contain the response bytes
  @param  responseLength The length of the desired response

  @returns  A boolean value indicating whether the operation succeeded or not
*/
/**************************************************************************/
bool cie_PN532::sendCommand(byte* command, const byte commandLength, byte* responseBuffer, byte* responseLength) {
  if (!_nfc->inDataExchange(command, commandLength, responseBuffer, responseLength) 
  || !hasSuccessStatusWord(responseBuffer, *responseLength)) {
    Serial.println("ERRORR");
    _nfc->PrintHex(responseBuffer, *responseLength);
    return false;
  }
  return true;
}


/**************************************************************************/
/*!
  @brief  Performs internal authentication

  @param  responseBuffer A pointer to the buffer which will contain the response for the internal authentication command
  @param  responseLength The length of the response
  
  @returns  A boolean value indicating whether the operation succeeded or not
*/
/**************************************************************************/
bool cie_PN532::internalAuthenticate_PkDhScheme(byte* responseBuffer, byte* responseLength) {
  byte internalAuthenticateCommand[] = {
    0x0C, //CLA
    0x88, //INS: INTERNAL AUTHENTICATE PK-DH scheme
    0x00, //P1: algorithm reference -> no further information (information available in the current SE)
    0x00, //P2: secret reference -> no further information (information available in the current SE)
    0x08, //Lc: Length of the rndIfd
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //rndIfd
    *responseLength //Number of bytes expected in the response
  };
  generateRandomBytes(internalAuthenticateCommand, 5, 8);
  bool success = sendCommand(internalAuthenticateCommand, sizeof(internalAuthenticateCommand), responseBuffer, responseLength);
  if (!success) {
    PN532DEBUGPRINT.println(F("Couldn't perform internal authentication"));
  }
  return success;
}


/**************************************************************************/
/*!
  @brief  Establishes a secure messaging context by performing mutual authentication
  
  @returns  A boolean value indicating whether the operation succeeded or not
*/
/**************************************************************************/
bool cie_PN532::establishSecureMessaging() {
  word snIccLength = EF_SN_ICC_LENGTH;
  byte* snIcc = new byte[snIccLength];

  byte rndIccLength = RND_LENGTH + STATUS_WORD_LENGTH;
  byte* rndIcc = new byte[rndIccLength];

  //Steps
  //1. Send a READ BINARY command for the SN.ICC Elementary File
  //2. Retrieval of the card key set for device authentication
  //3. Send a GET CHALLENGE command and retrieve the RND.ICC
  //4. Send a MUTUAL AUTHENTICATE command
  //5. Verify the MAC is correct
  bool success = true;
  if (!read_EF_SN_ICC(snIcc, &snIccLength)
   || !getChallenge(rndIcc, &rndIccLength)) {
    success = false;
  }
  if (!success) {
    PN532DEBUGPRINT.println(F("Couldn't establish a secure messaging context"));
  }

  delete [] snIcc;
  delete [] rndIcc;

  return success;
}


/**************************************************************************/
/*!
  @brief  Reads the binary content of an Elementary File given its Dedicated File and EFID or SFI identifier
	
  @param contentBuffer Pointer to the response data
  @param contentLength The length of the response
	
  @returns  A boolean value indicating whether the operation succeeded or not
*/
/**************************************************************************/
bool cie_PN532::getChallenge(byte* contentBuffer, byte* contentLength) {
  byte getChallengeCommand[] = {
    0x00, //CLA
    0x84, //INS: GET CHALLENGE
    0x00, //P1: not used
    0x00, //P2: not used
    0x08 //Le: Length of the rndIcc
  };
  bool success = sendCommand(getChallengeCommand, sizeof(getChallengeCommand), contentBuffer, contentLength);
  if (!success) {
    PN532DEBUGPRINT.println(F("Couldn't get challenge"));
  }
  *contentLength = RND_LENGTH;
  return success;
}


/**************************************************************************/
/*!
  @brief  Reads the binary content of an Elementary File given its Dedicated File and EFID or SFI identifier
	
  @param df The containing Dedicated File (either ROOT_MF or CIE_DF)
  @param filePath a structure indicating the parent Dedicated File (either ROOT_MF or CIE_DF), the selection mode (either SELECT_BY_EFID or SELECT_BY_SFI) and the file identifier (either a sfi or an efid)
  @param contentBuffer Pointer to the response data
  @param response The maximum desired length of the response
  @param lengthStrategy How to determine the length of the file (either FIXED_LENGTH or AUTODETECT_BER_LENGTH or AUTODETECT_ATR_LENGTH)
	
  @returns  A boolean value indicating whether the operation succeeded or not
*/
/**************************************************************************/
bool cie_PN532::readElementaryFile(cie_EFPath filePath, byte* contentBuffer, word* contentLength, const byte lengthStrategy) {
  //Some arguments passed around but more testable
  if (!determineLength(filePath, contentLength, lengthStrategy) ||
      !readBinaryContent(filePath, contentBuffer, READ_FROM_START, *contentLength)) {
    return false;
  }
  return true;
}


/**************************************************************************/
/*!
  @brief  Selects an Elementary file
  
  @param df The containing Dedicated File (either ROOT_MF or CIE_DF)
  @param efid The efid of the Elementary File (must be two bytes)
  
  @returns  A boolean value indicating whether the operation succeeded or not
*/
/**************************************************************************/
bool cie_PN532::ensureElementaryFileIsSelected(cie_EFPath filePath) {

  if (filePath.selectionMode != SELECT_BY_EFID) {
    PN532DEBUGPRINT.println(F("This method should be called just for EFID selection"));
    return false;
  }

  if (!ensureDedicatedFileIsSelected(filePath.df)) {
    return false;
  }

  //If it's already selected, no need to select it again 
  if (_currentElementaryFile == filePath.id) {
    return true;
  }

  byte efid[2] = { (byte) (filePath.id >> 8), (byte) (filePath.id & 0b11111111) };
  byte selectCommand[] = {
    0x00, //CLA
    0xA4, //INS: SELECT FILE
    0x02, //P1: Select by EFID
    0x0C, //P2: No data in response
    0x02, //Lc: Length of EFID
    efid[0], efid[1] //EFID
  };
  bool success = sendCommand(selectCommand, sizeof(selectCommand));
  if (!success) {
    PN532DEBUGPRINT.print(F("Couldn't select the EF by its EFID "));
    _nfc->PrintHex(efid, 2);
    return false;
  }
  _currentElementaryFile = filePath.id;
  return true;
}


/**************************************************************************/
/*!
  @brief  Ensures an elementary file is selected by its SDO ID, that is by sending an MSE mode SET (with AT template) command
	
  @param filePath a structure indicating the parent Dedicated File (either ROOT_MF or CIE_DF), the selection mode (either SELECT_BY_EFID or SELECT_BY_SFI) and the file identifier (either a sfi or an efid)
  
  @returns  A boolean value indicating whether the operation succeeded or not
*/
/**************************************************************************/
bool cie_PN532::ensureSdoIsSelected(cie_EFPath filePath) {
  if (filePath.selectionMode != SELECT_BY_SDOID) {
    PN532DEBUGPRINT.println(F("This method should be called just for SDO selection"));
    return false;
  }

  if (!ensureDedicatedFileIsSelected(filePath.df)) {
    return false;
  }

  //If it's already selected, no need to select it again 
  if (_currentElementaryFile == filePath.id) {
    return true;
  }
  byte selectCommand[] = {
    0x00, //CLA
    0x22, //INS: MSE mode SET
    0x41, //P1: Template AT
    0xA4, //P2
    0x06, //Lc: Length of the data (composed of two TLV triples)
    0x80, 0x01, 0x02, //First TLV triple: Selection of the RSA PKCS#1 - SHA1 with not data formatting
    0x84, 0x01, (byte)(0b10000000 | filePath.id) //Second TLV triple: SDO ID OR'ed with 0b10000000 to indicate it's in the currently selected DF
  };
  bool success = sendCommand(selectCommand, sizeof(selectCommand));
  if (!success) {
    PN532DEBUGPRINT.println(F("Couldn't select the EF by its SDO ID "));
    return false;
  }
  _currentElementaryFile = filePath.id;
  return true;
}


/**************************************************************************/
/*!
  @brief  Selects a dedicated file in the IAS application
  
  @param df The Dedicated File (either ROOT_MF or CIE_DF)
  
  @returns  A boolean value indicating whether the operation succeeded or not
*/
/**************************************************************************/
bool cie_PN532::ensureDedicatedFileIsSelected (const byte df) {
  //No need to re-select the same Dedicated File if it's already the selected one
  if (_currentDedicatedFile == df) {
    return true;
  } else {
    //We're chaging Dedicated File, the current Elementary File will be deselected to prevent id collision
    _currentElementaryFile = NULL_EF;
  }

  if (!selectIasApplication()) {
    return false;
  }
  
  switch (df) {
    case ROOT_MF:
    if (!selectRootMasterFile()) {
      return false;
    }
    break;

    case CIE_DF:
    if (!selectCieDedicatedFile()) {
      return false;
    }
    break;

    default:
      PN532DEBUGPRINT.println(F("The DF must be either ROOT_MF or CIE_DF"));
      return false;
  }
  _currentDedicatedFile = df;
  return true;
}


/**************************************************************************/
/*!
  @brief  Sets the length value of a currently selected Elementary File
  
  @param filePath a structure indicating the parent Dedicated File (either ROOT_MF or CIE_DF), the selection mode (either SELECT_BY_EFID or SELECT_BY_SFI) and the file identifier (either a sfi or an efid)
  @param contentLength The pointer to the length value
  @param lengthStrategy The chosen strategy by user (either AUTODETECT_BER_LENGTH or FIXED_LENGTH)

  @returns  A boolean value indicating whether the operation succeeded or not
*/
/**************************************************************************/
bool cie_PN532::determineLength(const cie_EFPath filePath, word* contentLength, const byte lengthStrategy) {
  switch (lengthStrategy) {
    case FIXED_LENGTH:
    //do nothing, size is already known
    break;

    case AUTODETECT_BER_LENGTH:
      cie_BerTriple* triple;
      if (!_berReader->readTriples(filePath, nullptr, contentLength, 1)) {
        return false;
      }
    break;

    case AUTODETECT_ATR_LENGTH:
      if (!_atrReader->detectLength(filePath, contentLength)) {
        return false;
      }
    break;

    default:
      PN532DEBUGPRINT.println(F("The length strategy must be either AUTODETECT_BER_LENGTH or FIXED_LENGTH"));
      return false;
  }
  return true;
}


/**************************************************************************/
/*!
  @brief Reads the binary content of the currently selected Elementary File

  @param filePath a structure indicating the parent Dedicated File (either ROOT_MF or CIE_DF), the selection mode (either SELECT_BY_EFID or SELECT_BY_SFI) and the file identifier (either a sfi or an efid)  
  @param contentBuffer The pointer to the data buffer
  @param contentLength The number of bytes to read

  @returns  A boolean value indicating whether the operation succeeded or not
*/
/**************************************************************************/
bool cie_PN532::readBinaryContent(const cie_EFPath filePath, byte* contentBuffer, word startingOffset, const word contentLength) {
  byte fileId;
  switch (filePath.selectionMode) {
    case SELECT_BY_EFID:
      if (!ensureElementaryFileIsSelected(filePath)) {
        return false;
      }
      fileId = 0x00; //Current selected file
    break;

    case SELECT_BY_SFI:
      if (!ensureDedicatedFileIsSelected(filePath.df)) {
        return false;
      }
      fileId = (byte) (filePath.id & 0b11111);
    break;

    default:
      PN532DEBUGPRINT.println(F("The selection mode must be either SELECT_BY_EFID or SELECT_BY_SFI"));
      return false;
  }
  bool success = false;
  word offset = startingOffset;
  byte preambleOctets = 2;
  do {
    word contentPageLength = clamp(contentLength+startingOffset-offset, PAGE_LENGTH);
    byte readCommand[] = {
      0x00, //CLA
      0xB1, //INS: READ BINARY (ODD INS)
      0x00, //P1: zeroes
      fileId, //P2: sfi to select or zeroes (i.e. keep current selected file)
      0x04, //Lc: data field is made of 4 bytes
      0x54, 0x02, //Data field: here comes an offset of 2 bytes
      (byte) (offset >> 8), (byte) (offset & 0b11111111), //the offset
      (byte) (contentPageLength + preambleOctets) //Le: bytes to be returned in the response
    };
    byte responseLength = ((byte) contentPageLength) + preambleOctets + STATUS_WORD_LENGTH;
    byte* responseBuffer = new byte[responseLength];  
    success = sendCommand(readCommand, 10, responseBuffer, &responseLength);
    //Copy data over to the buffer
    if (success) {
      //The read binary command with ODD INS incapsulated the response with two preamble octets. Don't include them, they're not part of the content.
      for (word i = preambleOctets; i < contentPageLength + preambleOctets; i++) {
        contentBuffer[offset - startingOffset + i - preambleOctets] = responseBuffer[i];
      }
    }
    delete [] responseBuffer;  
    offset += contentPageLength;
    
  } while(success && (offset < contentLength));  
  if (!success) {
    PN532DEBUGPRINT.println(F("Couldn't fetch the elementary file content"));
  }  
  return success;
}


/**************************************************************************/
/*!
  @brief Reads the key content (BER encoded modulo and exponent) from the indicated Elementary File

  @param filePath a structure indicating the parent Dedicated File (either ROOT_MF or CIE_DF), the selection mode (either SELECT_BY_EFID or SELECT_BY_SFI) and the file identifier (either a sfi or an efid)  
  @param key A pointer to a which object which will be populated with the modulo and exponent

  @returns  A boolean value indicating whether the operation succeeded or not
*/
/**************************************************************************/
bool cie_PN532::readKey(const cie_EFPath filePath, cie_Key* key) {
  //This is the fasted way but assumes we'll find a 2048-bit key and a 24-bit exponent valued 0x010001
  //TODO: use public methods of the cie_Key class to allocate memory for buffers
  //And make exponent and modulo readonly by using getter and setter methods
  key->exponentLength = 3;
  key->exponent = new byte[key->exponentLength] { 0x01, 0x00, 0x01 };
  key->moduloLength = 256;
  key->modulo = new byte[key->moduloLength];
  return readBinaryContent(filePath, key->modulo, 9, key->moduloLength);

  //TODO: proper BER parsing by using the cie_BerReader class
}


/**************************************************************************/
/*!
  @brief  Selects the ROOT Master File
	
  @returns  A boolean value indicating whether the operation succeeded or not
*/
/**************************************************************************/
bool cie_PN532::selectRootMasterFile(void) {
  byte command[] = { 
      0x00, //CLA
      0xA4, //INS: SELECT FILE
      0x00, //P1: Select root
      0x0C, //P2: No data in response field
      0x02, //Lc: Length of root id
      0x3F, 0x00 //root id
  };
  bool success = sendCommand(command, sizeof(command));
  if (!success) {
    PN532DEBUGPRINT.println(F("Couldn't select root"));
  }
  return success;
}


/**************************************************************************/
/*!
  @brief  Selects the IAS Application
	
  @returns  A value indicating whether the operation succeeded or not
*/
/**************************************************************************/
bool cie_PN532::selectIasApplication(void) {
  byte command[] = { 
      0x00, //CLA
      0xA4, //INS: SELECT FILE
      0x04, //P1: Select by AID
      0x0C, //P2: No data in response field
      0x0D, //Lc: Length of AID
      0xA0, 0x00, 0x00, 0x00, 0x30, 0x80, 0x00, 0x00, 0x00, 0x09, 0x81, 0x60, 0x01 //AID
  };
  bool success = sendCommand(command, sizeof(command));
  if (!success) {
    PN532DEBUGPRINT.println(F("Couldn't select the IAS application"));
  }
  return success;
}


/**************************************************************************/
/*!
  @brief  Selects the CIE DF
	
  @returns  A value indicating whether the operation succeeded or not
*/
/**************************************************************************/
bool cie_PN532::selectCieDedicatedFile(void) {
  byte command[] = { 
      0x00, //CLA
      0xA4, //INS: SELECT FILE
      0x04, //P1: Select by ID
      0x0C, //P2: No data in response field
      0x06, //Lc: length of ID
      0xA0, 0x00, 0x00, 0x00, 0x00, 0x39 //ID
  };
  bool success = sendCommand(command, sizeof(command));
  if (!success) {
    PN532DEBUGPRINT.println(F("Couldn't select the CIE DF"));
  }
  return success;
}


/**************************************************************************/
/*!
    @brief  Reads the EF contents by its SFI under the current DF
	
	  @param  response Pointer to the response data
	
    @returns  A value indicating whether the operation succeeded or not
*/
/**************************************************************************/
bool cie_PN532::hasSuccessStatusWord(byte* response, const word responseLength) {
  byte msByte = response[responseLength-2];
  byte lsByte = response[responseLength-1];
  bool success = msByte == 0x90 && lsByte == 0x00;
  if (success) {
    return true;
  }

  if (msByte == 0x62 && lsByte == 0x83) {
    PN532DEBUGPRINT.print(F("Warning selected file deactivated"));
  } else if (msByte == 0x62 && lsByte == 0x85) {
    PN532DEBUGPRINT.print(F("The selected file is in terminate state"));
  } else if (msByte == 0x62 && lsByte == 0x82) {
    PN532DEBUGPRINT.print(F("End of file/record reached before reading Le bytes"));
  } else if (msByte == 0x67 && lsByte == 0x00) {
    PN532DEBUGPRINT.print(F("Wrong length"));
  } else if (msByte == 0x69 && lsByte == 0x82) {
    PN532DEBUGPRINT.print(F("Security condition not satisfied"));
  } else if (msByte == 0x6A && lsByte == 0x82) {
    PN532DEBUGPRINT.print(F("File or application not found"));
  } else if (msByte == 0x6A && lsByte == 0x86) {
    PN532DEBUGPRINT.print(F("Incorrect parameters P1-P2"));
  } else if (msByte == 0x6A && lsByte == 0x87) {
    PN532DEBUGPRINT.print(F("Nc inconsistent with parameters P1-P2"));
  } else if (msByte == 0x6D && lsByte == 0x00) {
    PN532DEBUGPRINT.print(F("Instruction code not supported or invalid"));
  } else {
    PN532DEBUGPRINT.print(F("Unknown error"));
  }
  PN532DEBUGPRINT.print(F(" "));
  byte statusWord[2] = { msByte, lsByte };
  _nfc->PrintHex(statusWord, 2);
  
  return false;
}


/**************************************************************************/
/*!
    @brief  Ensures a value is less or equal than a max value
	
    @param  value The value that should be checked
    @param  maxValue The maximum value

    @returns The clamped value

*/
/**************************************************************************/
word cie_PN532::clamp(const word value, const byte maxValue) {
  if (value > maxValue) {
    return maxValue;
  } else {
    return value;
  }
}


/**************************************************************************/
/*!
    @brief  Populates a buffer with random generated bytes
	
    @param  buffer The pointer to a byte array
    @param  offset The starting offset in the buffer
    @param  length The number of random bytes to generate

*/
/**************************************************************************/
void cie_PN532::generateRandomBytes(byte* buffer, const word offset, const byte length) {
  randomSeed(analogRead(0)); //Use an unconnected analog pin as the random seed
  for (word i = offset; i<offset+length; i++) {
    buffer[i] = (byte) random(256);
  }
}


/**************************************************************************/
/*!
    @brief Frees resources
*/
/**************************************************************************/
cie_PN532::~cie_PN532()
{
    delete _atrReader;
    delete _berReader;
    delete _nfc;
}