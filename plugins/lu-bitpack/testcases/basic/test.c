
struct TestStruct {
   int a;
   int b;
   int c;
};

typedef char player_name_type [7];

typedef struct {
   player_name_type challenger;
   int health;
} TestTypedefStruct;

void ShowChallenger(TestTypedefStruct* info) {
   int local_variable = 5;
   
   #pragma lu_bitpack debug_dump_identifier local_variable
}

#pragma lu_bitpack debug_dump_identifier TestStruct
#pragma lu_bitpack debug_dump_identifier player_name_type
#pragma lu_bitpack debug_dump_identifier TestTypedefStruct
#pragma lu_bitpack debug_dump_function ShowChallenger