mov(8)          g123<1>UD       g1<8,8,1>UD                     { align1 WE_all 1Q };
mov(8)          g124<1>F        0x40c00000F      /* 6F */       { align1 1Q };
mov(8)          g14<1>UD        0x00000000UD                    { align1 1Q };
mov(8)          g17<1>F         g12<8,8,1>F                     { align1 1Q };
mov.sat(8)      g124<1>F        g8<8,8,1>F                      { align1 1Q };
mov(8)          g61<2>D         g22<8,8,1>D                     { align1 1Q };
mov(8)          g21<1>D         g59<8,4,2>UD                    { align1 1Q };
mov(8)          g4<1>D          -1D                             { align1 1Q };
mov.nz.f0.0(8)  null<1>D        g4<8,8,1>D                      { align1 1Q };
mov(1)          g2.2<1>UD       0x00000000UD                    { align1 WE_all 1N };
mov(4)          g114<1>F        g2.3<8,2,4>F                    { align1 WE_all 1N };
mov(8)          g126<1>F        g4<8,8,1>D                      { align1 1Q };
mov(16)         g124<1>F        g4<8,8,1>D                      { align1 1H };
mov(16)         g120<1>F        g124<8,8,1>F                    { align1 1H };
mov(16)         g124<1>F        0x0F             /* 0F */       { align1 1H };
mov(16)         g124<1>D        1065353216D                     { align1 1H };
mov.nz.f0.0(16) null<1>D        g2<0,1,0>D                      { align1 1H };
mov(8)          g3<1>UW         0x76543210V                     { align1 WE_all 1Q };
mov(16)         g20<1>UD        g0.1<0,1,0>UD                   { align1 1H };
mov(16)         g6<1>D          g3<8,8,1>UW                     { align1 1H };
mov(8)          g1<1>D          g4<8,8,1>D                      { align1 2Q };
mov(8)          g5<1>D          0D                              { align1 2Q };
mov(8)          g2<1>F          g6<8,4,1>UW                     { align1 1Q };
mov(8)          g7<1>D          g2<8,8,1>F                      { align1 1Q };
mov(16)         g2<1>F          g10<8,4,1>UW                    { align1 1H };
mov(16)         g11<1>D         g2<8,8,1>F                      { align1 1H };
mov(8)          g80<1>DF        g5<0,1,0>DF                     { align1 1Q };
mov(8)          g92<2>UD        g6.4<0,1,0>UD                   { align1 1Q };
mov(8)          g62<1>Q         0xbff0000000000000Q             { align1 1Q };
mov(8)          g92<2>F         g92<4,4,1>DF                    { align1 1Q };
mov(8)          g92<1>DF        g95<4,4,1>F                     { align1 1Q };
mov(8)          g106<1>DF       g2<0,1,0>F                      { align1 2Q };
mov(8)          g48<1>Q         0xbff0000000000000Q             { align1 2Q };
mov(8)          g127<1>UD       g106.1<8,4,2>UD                 { align1 2Q };
mov(8)          g11<2>F         g7<4,4,1>DF                     { align1 2Q };
mov(8)          g33<1>D         g34<8,4,2>UD                    { align1 2Q };
mov(8)          g6<2>UD         0x00000000UD                    { align1 2Q };
mov(8)          g2<1>UW         0x76543210UV                    { align1 1Q };
mov(8)          g12<1>UD        g2<8,8,1>UW                     { align1 1Q };
mov(8)          g7<1>UD         0x00080000UD                    { align1 WE_all 1Q };
mov(1)          g2<1>F          0x3e800000F      /* 0.25F */    { align1 WE_all 1N };
mov(8)          g15<1>F         g11<8,8,1>UD                    { align1 1Q };
mov(1)          f0.1<1>UW       g1.14<0,1,0>UW                  { align1 WE_all 1N };
mov(8)          g18<1>UD        g2<8,8,1>D                      { align1 1Q };
mov(16)         g18<1>UD        g26<8,8,1>D                     { align1 1H };
mov(16)         g120<1>D        g34<8,8,1>D                     { align1 1H };
mov(8)          g8<1>Q          g13<4,4,1>Q                     { align1 1Q };
mov(8)          g21<1>UD        g0<8,8,1>UD                     { align1 WE_all 2Q };
mov(8)          g23<1>F         g6<0,1,0>F                      { align1 2Q };
mov(1)          g21.2<1>UD      0x000003f2UD                    { align1 WE_all 3N };
mov.nz.f0.0(8)  g19<1>D         g3<8,4,2>UD                     { align1 1Q };
mov(1)          f1<1>UD         g1.7<0,1,0>UD                   { align1 WE_all 1N };
mov.sat(8)      g126<1>F        0x0F             /* 0F */       { align1 1Q };
mov.sat(8)      g124<1>F        -g36<8,8,1>D                    { align1 1Q };
mov(8)          g41<1>F         0x0F             /* 0F */       { align1 2Q };
mov(8)          g42<1>UD        g11<8,8,1>D                     { align1 2Q };
mov(16)         g86<1>UD        g88<8,8,1>UD                    { align1 WE_all 1H };
mov.sat(16)     g120<1>F        g2<0,1,0>F                      { align1 1H };
mov(16)         g2<1>F          g18<8,8,1>UD                    { align1 1H };
mov(8)          g4<1>UD         0x0F             /* 0F */       { align1 1Q };
mov(8)          g8<1>DF         g2<0,1,0>D                      { align1 1Q };
mov(16)         g8<1>UD         0x00000000UD                    { align1 1H };
mov.nz.f0.0(8)  g4<1>F          -(abs)g2<0,1,0>F                { align1 1Q };
(+f0.0) mov(8)  g4<1>F          0xbf800000F      /* -1F */      { align1 1Q };
mov.nz.f0.0(16) g4<1>F          -(abs)g2<0,1,0>F                { align1 1H };
(+f0.0) mov(16) g4<1>F          0xbf800000F      /* -1F */      { align1 1H };
mov(1)          f1<1>UD         g1.7<0,1,0>UD                   { align1 WE_all 3N };
mov(8)          g32<1>DF        g2<0,1,0>DF                     { align1 2Q };
mov(8)          g5<1>F          g2<0,1,0>HF                     { align1 1Q };
mov(16)         g6<1>F          g2<0,1,0>HF                     { align1 1H };
mov(8)          g7<1>UD         g2<0,1,0>F                      { align1 1Q };
mov(16)         g15<1>UD        g11<8,8,1>F                     { align1 1H };
mov(16)         g19<1>UD        g15<16,8,2>UW                   { align1 1H };
mov(1)          g19<1>UD        g[a0 64]<0,1,0>UD               { align1 WE_all 1N };
mov(16)         g23<1>UD        g21<32,8,4>UB                   { align1 1H };
mov(8)          g7<1>DF         0x0000000000000000DF /* 0DF */  { align1 1Q };
mov(8)          g5<1>F          0x0F             /* 0F */       { align1 WE_all 1Q };
mov(16)         g4<1>UD         0x00000000UD                    { align1 WE_all 1H };
mov(8)          g5<2>UD         g2<0,1,0>DF                     { align1 1Q };
mov(8)          g10<2>UD        g2<0,1,0>DF                     { align1 2Q };
mov(8)          g3<1>DF         g2<0,1,0>UD                     { align1 1Q };
mov(8)          g3<1>DF         g2<0,1,0>UD                     { align1 2Q };
mov(1)          f0<1>UW         0x0000UW                        { align1 WE_all 1N };
mov(1)          g1<1>D          0D                              { align1 WE_all 1N };
(+f0.0.any16h) mov(1) g1<1>D    -1D                             { align1 WE_all 1N };
mov(8)          g9<1>F          g2<0,1,0>W                      { align1 1Q };
mov(8)          g7<1>UQ         g4<4,4,1>UQ                     { align1 1Q };
mov(16)         g11<1>UD        0x0F             /* 0F */       { align1 1H };
mov(8)          g5<2>D          g2<0,1,0>DF                     { align1 1Q };
mov(8)          g10<2>D         g2<0,1,0>DF                     { align1 2Q };
mov(1)          f1<1>UW         f0.1<0,1,0>UW                   { align1 WE_all 1N };
mov(1)          f1<1>UW         f0.1<0,1,0>UW                   { align1 WE_all 3N };
mov(16)         g4<1>D          0D                              { align1 2H };
mov(8)          g14<1>UD        g13<32,8,4>UB                   { align1 1Q };
mov(16)         g124<1>UD       g15<8,8,1>UD                    { align1 2H };
mov(16)         g118<1>D        g122<8,8,1>UW                   { align1 2H };
mov(16)         g101<1>UD       0x00000001UD                    { align1 2H };
mov(1)          g4<2>UW         0x00000000UD                    { align1 WE_all 1N };
mov(8)          g4<1>UD         f0<0,1,0>UW                     { align1 1Q };
mov(8)          g8<1>D          g2<8,8,1>UW                     { align1 1Q };
mov(16)         g4<1>UD         f0<0,1,0>UW                     { align1 1H };
mov(8)          g3<1>DF         -g2<0,1,0>D                     { align1 2Q };
mov(8)          g5<1>F          g2<0,1,0>B                      { align1 1Q };
mov(16)         g6<1>F          g2<0,1,0>B                      { align1 1H };
mov(8)          g4<1>DF         0x0000000000000000DF /* 0DF */  { align1 2Q };
mov.nz.f0.0(8)  g16<1>D         g17<8,4,2>UD                    { align1 2Q };
mov(8)          g34<1>UW        0x76543210V                     { align1 1Q };
mov(8)          g8<1>UD         48D                             { align1 1Q };
mov(16)         g8<1>UD         0D                              { align1 1H };
mov(8)          g7<2>HF         g2.1<0,1,0>F                    { align1 1Q };
mov(1)          g5<1>D          g[a0 96]<0,1,0>D                { align1 WE_all 1N };
(+f0.0.any8h) mov(1) g2<1>D     -1D                             { align1 WE_all 1N };
mov(8)          g9<1>UD         0D                              { align1 WE_all 1Q };
mov(8)          g2<2>UW         g9<8,8,1>F                      { align1 1Q };
mov(8)          g3<1>UW         g2<16,8,2>UW                    { align1 1Q };
mov(8)          g12<1>UW        g8<16,8,2>UW                    { align1 WE_all 1Q };
mov.sat(16)     g13<1>F         0x3f800000F      /* 1F */       { align1 1H };
mov(16)         g19<2>UW        g17<8,8,1>F                     { align1 1H };
mov(16)         g4<1>UW         g13<16,8,2>UW                   { align1 WE_all 1H };
mov.nz.f0.0(8)  null<1>D        0x00000000UD                    { align1 1Q };
mov.nz.f0.0(16) null<1>D        0x00000000UD                    { align1 1H };
mov(4)          g3<1>UD         tm0<4,4,1>UD                    { align1 WE_all 1N };
(+f0.0.all16h) mov(1) g1<1>D    -1D                             { align1 WE_all 1N };
mov(8)          g9<1>F          g2<0,1,0>UB                     { align1 1Q };
mov(16)         g6<1>F          g2<0,1,0>UB                     { align1 1H };
mov(16)         g10<2>HF        g4<8,8,1>F                      { align1 1H };
mov.z.f0.0(8)   null<1>UD       g2<8,8,1>UD                     { align1 1Q };
mov.sat(8)      g125<1>F        g9<8,8,1>UD                     { align1 1Q };
mov.z.f0.0(16)  g1<1>UD         g0.7<0,1,0>UD                   { align1 1H };
mov.z.f0.0(8)   g18<1>D         g17<8,8,1>F                     { align1 1Q };
mov(16)         g35<1>F         g15<16,8,2>W                    { align1 1H };
mov(8)          g23<1>Q         g26<4,4,1>Q                     { align1 2Q };
mov(8)          g2<1>D          0x00000000UD                    { align1 1Q };
mov(16)         g2<1>D          0x00000000UD                    { align1 1H };
(+f0.0.all8h) mov(1) g7<1>D     -1D                             { align1 WE_all 1N };
mov(8)          g127<1>UB       g2<0,1,0>UB                     { align1 WE_all 1Q };
mov.z.f0.0(8)   null<1>D        g24<8,8,1>F                     { align1 1Q };
mov.z.f0.0(16)  null<1>D        g76<8,8,1>F                     { align1 1H };
mov(16)         g7<1>D          g2<16,8,2>B                     { align1 1H };
