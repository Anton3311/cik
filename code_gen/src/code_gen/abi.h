#ifndef ABI_H
#define ABI_H

typedef enum {
	CALL_CONV_CDECL,
} CallingConvention;

typedef enum {
	ABI_PARAM_NORMAL,
	ABI_PARAM_STRUCT,
	ABI_PARAM_RETURN_LOCATION,
} AbiParamKind;

typedef struct {
	AbiParamKind kind;
	union {
		uint32_t struct_size;
	};
} AbiParam;

typedef struct {
	CallingConvention call_conv;
	uint32_t param_count;

	AbiParam* params;

	// Optional return
	AbiParam* returns;
} AbiSignature;

#endif
