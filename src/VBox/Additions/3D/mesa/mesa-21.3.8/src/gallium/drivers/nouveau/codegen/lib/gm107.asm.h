uint64_t gm107_builtin_code[] = {
/* 0x0000: gm107_div_u32 */
	0x001f9801fc21ff0d,
	0x5c30000000170002,
	0x3847040001f70202,
	0x3898078000170003,
	0x003c1800e1e007e1,
	0x5c48000000270302,
	0x5ce0200000170a01,
	0x5c38000000270103,
	0x003c1801e0c00f06,
	0x5a40010000370202,
	0x5c38000000270103,
	0x5a40010000370202,
	0x003c1801e0c00f06,
	0x5c38000000270103,
	0x5a40010000370202,
	0x5c38000000270103,
	0x00241801e0c00f06,
	0x5a40010000370202,
	0x5c38000000270103,
	0x5a40010000370202,
	0x00443c0120c017e6,
	0x5c98078000070003,
	0x5c38008000270000,
	0x5ce0200000170a02,
	0x001f8401fda01f06,
	0x5a00018000070101,
	0x5b6c038000270107,
	0x5c11000000200101,
	0x001f8400fda007e5,
	0x3810000000100000,
	0x5b6c038000200107,
	0x5c11000000200101,
	0x001fbc00fde007e1,
	0x3810000000100000,
	0xe32000000007000f,
	0x50b0000000070f00,
/* 0x0120: gm107_div_s32 */
	0x001c0400fc21ffed,
	0x5b6303800ff70017,
	0x5b6341000ff7011f,
	0x5ce2000000073a00,
	0x005f8402e5a0072f,
	0x5ce2000000173a01,
	0x5c30000000170002,
	0x3847040001f70202,
	0x001cbc00fc2007e6,
	0x3898078000170003,
	0x5c48000000270302,
	0x5ce0200000170a01,
	0x005c9802e4c01726,
	0x5c38000000270103,
	0x5a40010000370202,
	0x5c38000000270103,
	0x005c9802e4c01726,
	0x5a40010000370202,
	0x5c38000000270103,
	0x5a40010000370202,
	0x005c9802e4c01726,
	0x5c38000000270103,
	0x5a40010000370202,
	0x5c38000000270103,
	0x00441805fc401226,
	0x5a40010000370202,
	0x5c98078000070003,
	0x5c38008000270000,
	0x007fb405e0c0122f,
	0x5ce0200000170a02,
	0x5a00018000070101,
	0x5b6c038000270107,
	0x001fb400fca007e1,
	0x5c11000000200101,
	0x3810000000100000,
	0x5b6c038000200107,
	0x001c3c00fc4007e1,
	0x5c11000000200101,
	0x3810000000100000,
	0x5ce0200000033a00,
	0x001fbc03fde0072f,
	0x5ce0200000123a01,
	0xe32000000007000f,
	0x50b0000000070f00,
/* 0x0280: gm107_rcp_f64 */
	0x001f8000fc0007e0,
	0x38000000b1470102,
	0x1c0ffffffff70203,
	0xe29000000e000000,
	0x001f8000fc0007e0,
	0x366803807fd70307,
	0x5c9807800ff70003,
	0xf0f800000008000f,
	0x001f8000fc0007e0,
	0x010ffffffff7f003,
	0x368c03fff0070087,
	0xe24000000188000f,
	0x001f8000fc0007e0,
	0x0420008000070101,
	0xf0f800000007000f,
/* 0x02f8: rcp_inf_or_denorm_or_zero */
	0x0407ff0000070104,
	0x001f8000fc0007e0,
	0x5b6503800ff70407,
	0xe24000000200000f,
	0x0447ff0000070101,
	0x001f8000fc0007e0,
	0x5c9807800ff70000,
	0xf0f800000007000f,
/* 0x0338: rcp_denorm_or_zero */
	0x5b8c03800ff70087,
	0x001f8000fc0007e0,
	0xe24000000100000f,
	0x0427ff0000070101,
	0xf0f800000007000f,
/* 0x0360: rcp_denorm */
	0x001f8000fc0007e0,
	0x3880004350070000,
	0x3898078003670003,
	0xf0f800000007000f,
/* 0x0380: rcp_rejoin */
	0x001f8000fc0007e0,
	0x5b6303800ff70307,
	0xe24000001c00000f,
	0x38000000b1470102,
	0x001f8000fc0007e0,
	0x040800fffff70107,
	0x1c03ff0000070707,
	0x5c98078000070006,
	0x001f8000fc0007e0,
	0x5ca8100000670e05,
	0x5080000000470504,
	0x010bf8000007f000,
	0x001f8000fc0007e0,
	0x5980000000570405,
	0x5981020000470500,
	0x5ca8000000070b00,
	0x001f8000fc0007e0,
	0x5ca8200000670f06,
	0x38a8003f80070b08,
	0x5b70040000070604,
	0x001f8000fc0007e0,
	0x5b70000000470000,
	0x5b70040000070604,
	0x5b70000000470000,
	0x001f8000fc0007e0,
	0x5b70040000070604,
	0x5b70000000470000,
	0x5b70040000070604,
	0x001f8000fc0007e0,
	0x5b70000000470000,
	0x381200003ff70202,
	0x5c10000000370204,
	0x001f8000fc0007e0,
	0x38000000b1470103,
	0x5c10000000470303,
	0x1c0ffffffff70302,
	0x001f8000fc0007e0,
	0x366203807fe70207,
	0xe24000000208000f,
	0x3848000001470404,
	0x001f8000fc0007e0,
	0x5c10000000170401,
	0xe24000000807000f,
/* 0x04d8: rcp_result_inf_or_denorm */
	0x366d03807ff70307,
	0x001f8000fc0007e0,
	0xe24000000288000f,
	0x0408000000070101,
	0x5c9807800ff70000,
	0x001f8000fc0007e0,
	0x1c07ff0000070101,
	0xe24000000407000f,
/* 0x0518: rcp_result_denorm */
	0x5b6a03800ff70307,
	0x001f8000fc0007e0,
	0x040800fffff70101,
	0x38a8003e80000b06,
	0x38a8003f00080b06,
	0x001f8000fc0007e0,
	0x1c00010000070101,
	0x5c80000000670000,
/* 0x0558: rcp_end */
	0xe32000000007000f,
/* 0x0560: gm107_rsq_f64 */
	0x001fb401fda1ff0d,
	0x368c03fff0070087,
	0x0420008000000101,
	0x0407fffffff70102,
	0x001fb400fda007ed,
	0x38000000b1470103,
	0x366603800027030f,
	0x5c47020000270002,
	0x001fb401e1a0070d,
	0x3880004350010000,
	0x5080000000770105,
	0x365a03807ff70306,
	0x001fb400fda007ed,
	0x5c47000000670202,
	0x5b6a03800ff70207,
	0xe24000000400000f,
	0x003fb400fda007ed,
	0x0408000000070101,
	0x5c9807800ff70000,
	0x5c47020000570101,
	0x001fbc00fde007ed,
	0xe32000000007000f,
	0x50b0000000070f00,
	0x50b0000000070f00,
/* 0x0620: rsq_norm */
	0x0060b400e5a007ed,
	0x5c9807800ff70004,
	0x38a8003f00070b08,
	0x5c80000000870002,
	0x003c3401e1a01f0d,
	0x5c80000000470200,
	0x5b71040000470006,
	0x5b70020000670404,
	0x003c3401e1a00f0d,
	0x5c80000000470200,
	0x5b71040000470006,
	0x5b70020000670404,
	0x003c3401e1a00f0d,
	0x5c80000000470200,
	0x5b71040000470006,
	0x5b70020000670404,
	0x003c3401e1a00f0d,
	0x5c80000000470200,
	0x5b71040000470006,
	0x5b70020000670404,
	0x001fb401fda00f0d,
	0x38800041a0010404,
	0x5c98078000570001,
	0x5c98078000470000,
	0x001fbc00fde007ed,
	0xe32000000007000f,
	0x50b0000000070f00,
	0x50b0000000070f00,
};

uint64_t gm107_builtin_offsets[] = {
	0x0000000000000000,
	0x0000000000000120,
	0x0000000000000280,
	0x0000000000000560,
};
