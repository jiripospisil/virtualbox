VERT

DCL IN[0]
DCL IN[1]
DCL OUT[0], POSITION
DCL OUT[1], COLOR

DCL TEMP[0]

IMM FLT32 { 1.0, 0.0, 0.0, 0.0 }
IMM FLT32 { 0.5, 0.0, 0.0, 0.0 }

ADD TEMP[0], IN[0], IMM[0]
LG2 TEMP[0].x, TEMP[0].xxxx
ADD OUT[0], TEMP[0], IMM[1]
MOV OUT[1], IN[1]

END
