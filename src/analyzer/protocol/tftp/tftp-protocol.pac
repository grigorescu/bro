
enum Opcodes {
	RRQ    = 1,
	WRQ    = 2,
	DATA   = 3,
	ACK    = 4,
	ERROR  = 5
};

type TFTP_Message = record {
	opcode:   uint16;
	op:       case opcode of {
		RRQ     -> read_request    : ReadRequest;
		WRQ     -> write_request   : WriteRequest;
		DATA    -> data            : DataChunk;
		ACK     -> acknowledgment  : Acknowledgment;
		ERROR   -> error           : Error;
		default -> failure         : Failure;
	};
	: bytestring &restofdata;
} &byteorder=bigendian;

type TFTP_STRING = RE/[^\x00]+/;

type ReadRequest = record {
	file: TFTP_STRING;
	:     uint8;
	type: TFTP_STRING;
	:     uint8;
};

type WriteRequest = record {
	file: TFTP_STRING;
	:     uint8;
	type: TFTP_STRING;
	:     uint8;
};

type DataChunk = record {
	block: uint16;
	data:  bytestring &restofdata;
};

type Acknowledgment = record {
	block: uint16;
};

type Error = record {
	errcode: uint16;
	errmsg:  TFTP_STRING;
	: 	 uint8;
};

type Failure = record {
	: empty;
};