# Test instructions using registers $1-$18
ADDI $1, $0, 10
ADDI $2, $0, 20
ADD $3, $1, $2
SUB $4, $2, $1
MUL $5, $1, $2
AND $6, $1, $2
OR $7, $1, $2
ADDI $8, $0, 5
ADDI $9, $0, 15
SLL $10, $8, 2
SRL $11, $9, 1
ADD $12, $3, $4
SUB $13, $5, $6
MUL $14, $7, $8
AND $15, $9, $10
OR $16, $11, $12
ADDI $17, $0, 100
SW $17, 0($0)
LW $18, 0($0)
HALT

