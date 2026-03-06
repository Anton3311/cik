typedef int int32;
typedef uint8_t bool;

typedef struct Hello {
	int a;
	struct World {

	} world;
} Hello;

typedef enum TokenKind {
	TOKEN_EOF,
	TOKEN_IDENT,
} TokenKind;

enum Type {
	INT,
	FLOAT
};
