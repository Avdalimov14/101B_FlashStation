#include "NdefRecord.h"
#include "esp_log.h"

NdefRecord::NdefRecord()
{
    //Serial.println("NdefRecord Constructor 1");
    _tnf = 0;
    _typeLength = 0;
    _payloadLength = 0;
    _idLength = 0;
    _type = (byte *)NULL;
    _payload = (byte *)NULL;
    _id = (byte *)NULL;
}

NdefRecord::NdefRecord(const NdefRecord& rhs)
{
    //Serial.println("NdefRecord Constructor 2 (copy)");

    _tnf = rhs._tnf;
    _typeLength = rhs._typeLength;
    _payloadLength = rhs._payloadLength;
    _idLength = rhs._idLength;
    _type = (byte *)NULL;
    _payload = (byte *)NULL;
    _id = (byte *)NULL;

    if (_typeLength)
    {
        _type = (byte*)malloc(_typeLength);
        memcpy(_type, rhs._type, _typeLength);
    }

    if (_payloadLength)
    {
        _payload = (byte*)malloc(_payloadLength);
        memcpy(_payload, rhs._payload, _payloadLength);
    }

    if (_idLength)
    {
        _id = (byte*)malloc(_idLength);
        memcpy(_id, rhs._id, _idLength);
    }

}

// TODO NdefRecord::NdefRecord(tnf, type, payload, id)

NdefRecord::~NdefRecord()
{
//    Serial.println("NdefRecord Destructor");
    if (_typeLength)
    {
        free(_type);
    }

    if (_payloadLength)
    {
        free(_payload);
    }

    if (_idLength)
    {
        free(_id);
    }
}

NdefRecord& NdefRecord::operator=(const NdefRecord& rhs)
{
//    Serial.println("NdefRecord ASSIGN");

    if (this != &rhs)
    {
        // free existing
        if (_typeLength)
        {
            free(_type);
        }

        if (_payloadLength)
        {
            free(_payload);
        }

        if (_idLength)
        {
            free(_id);
        }

        _tnf = rhs._tnf;
        _typeLength = rhs._typeLength;
        _payloadLength = rhs._payloadLength;
        _idLength = rhs._idLength;

        if (_typeLength)
        {
            _type = (byte*)malloc(_typeLength);
            memcpy(_type, rhs._type, _typeLength);
        }

        if (_payloadLength)
        {
            _payload = (byte*)malloc(_payloadLength);
            memcpy(_payload, rhs._payload, _payloadLength);
        }

        if (_idLength)
        {
            _id = (byte*)malloc(_idLength);
            memcpy(_id, rhs._id, _idLength);
        }
    }
    return *this;
}

// size of records in bytes
int NdefRecord::getEncodedSize()
{
    int size = 2; // tnf + typeLength
    if (_payloadLength > 0xFF)
    {
        size += 4;
    }
    else
    {
        size += 1;
    }

    if (_idLength)
    {
        size += 1;
    }

    size += (_typeLength + _payloadLength + _idLength);

    return size;
}

void NdefRecord::encode(byte *data, bool firstRecord, bool lastRecord)
{
    // assert data > getEncodedSize()

    uint8_t* data_ptr = &data[0];

    *data_ptr = getTnfByte(firstRecord, lastRecord);
    data_ptr += 1;

    *data_ptr = _typeLength;
    data_ptr += 1;

    if (_payloadLength <= 0xFF) {  // short record
        *data_ptr = _payloadLength;
        data_ptr += 1;
    } else { // long format
        // 4 bytes but we store length as an int
        data_ptr[0] = 0x0; // (_payloadLength >> 24) & 0xFF;
        data_ptr[1] = 0x0; // (_payloadLength >> 16) & 0xFF;
        data_ptr[2] = (_payloadLength >> 8) & 0xFF;
        data_ptr[3] = _payloadLength & 0xFF;
        data_ptr += 4;
    }

    if (_idLength)
    {
        *data_ptr = _idLength;
        data_ptr += 1;
    }

    //Serial.println(2);
    memcpy(data_ptr, _type, _typeLength);
    data_ptr += _typeLength;

    memcpy(data_ptr, _payload, _payloadLength);
    data_ptr += _payloadLength;

    if (_idLength)
    {
        memcpy(data_ptr, _id, _idLength);
        data_ptr += _idLength;
    }
}

byte NdefRecord::getTnfByte(bool firstRecord, bool lastRecord)
{
    int value = _tnf;

    if (firstRecord) { // mb
        value = value | 0x80;
    }

    if (lastRecord) { //
        value = value | 0x40;
    }

    // chunked flag is always false for now
    // if (cf) {
    //     value = value | 0x20;
    // }

    if (_payloadLength <= 0xFF) {
        value = value | 0x10;
    }

    if (_idLength) {
        value = value | 0x8;
    }

    return value;
}

byte NdefRecord::getTnf()
{
    return _tnf;
}

void NdefRecord::setTnf(byte tnf)
{
    _tnf = tnf;
}

unsigned int NdefRecord::getTypeLength()
{
    return _typeLength;
}

int NdefRecord::getPayloadLength()
{
    return _payloadLength;
}

unsigned int NdefRecord::getIdLength()
{
    return _idLength;
}

String NdefRecord::getType()
{
    char type[_typeLength + 1];
    memcpy(type, _type, _typeLength);
    type[_typeLength] = '\0'; // null terminate
    return String(type);
}

// this assumes the caller created type correctly
void NdefRecord::getType(uint8_t* type)
{
    memcpy(type, _type, _typeLength);
}

void NdefRecord::setType(const byte * type, const unsigned int numBytes)
{
	ESP_LOGD("DEBUGGGGG", "numBytes is %d",(int)numBytes );
	if (_typeLength == 0) {
		if(_typeLength)
		{
			free(_type);
		}
		ESP_LOGD("DEBUGGGGG", "_type is %d",(int)_type );
		_type = (uint8_t*)malloc(numBytes);
		ESP_LOGD("DEBUGGGGG", "_type is %d",(int)_type);
		memcpy(_type, type, numBytes);
		ESP_LOGD("DEBUGGGGG", "_type is %d ",(int)_type);
		_typeLength = numBytes;
	}
}

// assumes the caller sized payload properly
void NdefRecord::getPayload(byte *payload)
{
    memcpy(payload, _payload, _payloadLength);
}

void NdefRecord::setPayload(const byte * payload, const int numBytes)
{
	ESP_LOGD("DEBUGGGGG", "numBytes is %d",(int)_payloadLength );
	if (_payloadLength == 0 || _payloadLength > 80) {
		if (_payloadLength)
		{
			free(_payload);
		}
		ESP_LOGD("DEBUGGGGG", "_payload is %d", (int)_payload);
		_payload = (byte*)malloc(numBytes);
		if (_payload != 0){
			ESP_LOGD("DEBUGGGGG", "_payload is %d", (int)_payload);
			memcpy(_payload, payload, numBytes);
			ESP_LOGD("DEBUGGGGG", "_payload is %d", (int)_payload);
			_payloadLength = numBytes;
		}
	}
}

String NdefRecord::getId()
{
    char id[_idLength + 1];
    memcpy(id, _id, _idLength);
    id[_idLength] = '\0'; // null terminate
    return String(id);
}

void NdefRecord::getId(byte *id)
{
    memcpy(id, _id, _idLength);
}

void NdefRecord::setId(const byte * id, const unsigned int numBytes)
{
    if (_idLength)
    {
        free(_id);
    }

    _id = (byte*)malloc(numBytes);
    memcpy(_id, id, numBytes);
    _idLength = numBytes;
}

void NdefRecord::print()
{
    ESP_LOGI("NFC LIBS", "  NDEF Record");
    ESP_LOGI("NFC LIBS", "    TNF 0x%d ", _tnf);//Serial.print(_tnf, HEX);Serial.print(" ");
    switch (_tnf) {
    case TNF_EMPTY:
    	ESP_LOGI("NFC LIBS", "Empty");
        break;
    case TNF_WELL_KNOWN:
    	ESP_LOGI("NFC LIBS", "Well Known");
        break;
    case TNF_MIME_MEDIA:
        ESP_LOGI("NFC LIBS", "Mime Media");
        break;
    case TNF_ABSOLUTE_URI:
    	ESP_LOGI("NFC LIBS", "Absolute URI");
        break;
    case TNF_EXTERNAL_TYPE:
    	ESP_LOGI("NFC LIBS", "External");
        break;
    case TNF_UNKNOWN:
    	ESP_LOGI("NFC LIBS", "Unknown");
        break;
    case TNF_UNCHANGED:
    	ESP_LOGI("NFC LIBS", "Unchanged");
        break;
    case TNF_RESERVED:
    	ESP_LOGI("NFC LIBS", "Reserved");
        break;
    default:
    	ESP_LOGI("NFC LIBS", " ");
    	break;
    }
    ESP_LOGI("NFC LIBS", "    Type Length 0x%d %d", _typeLength, _typeLength);//Serial.print(_typeLength, HEX);Serial.print(" ");Serial.println(_typeLength);
    ESP_LOGI("NFC LIBS", "    Payload Length 0x%d %d", _payloadLength, _payloadLength);//Serial.print(_payloadLength, HEX);;Serial.print(" ");Serial.println(_payloadLength);
    if (_idLength)
    {
    	ESP_LOGI("NFC LIBS", "    Id Length 0x%d", _idLength); //Serial.println(_idLength, HEX);
    }
    ESP_LOGI("NFC LIBS", "    Type ");
    esp_log_buffer_hex("NFC LIBS", _type, _typeLength);//PrintHexChar(_type, _typeLength);
//     TODO chunk large payloads so this is readable
    ESP_LOGI("NFC LIBS", "    Payload ");
    esp_log_buffer_hex("NFC LIBS", _payload, _payloadLength);//PrintHexChar(_payload, _payloadLength);
    if (_idLength)
    {
    	ESP_LOGI("NFC LIBS", "    Id ");
    	esp_log_buffer_hex("NFC LIBS", _id, _idLength);//PrintHexChar(_id, _idLength);
    }
    ESP_LOGI("NFC LIBS", "    Record is %d bytes", getEncodedSize()); //Serial.print(getEncodedSize());Serial.println(" bytes");

}
