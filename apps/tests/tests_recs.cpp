/**
 *  Copyright 2015 Mike Reed
 */

#include "../../include/GCanvas.h"
#include "../../include/GBitmap.h"
#include "../../include/GColor.h"
#include "../../include/GPoint.h"
#include "../../include/GRect.h"
#include "tests.h"

#include "tests_pa1.inc"
#include "tests_pa2.inc"
#include "tests_pa3.inc"
#include "tests_pa4.inc"
#include "tests_pa5.inc"

///////////////////////////////////////////////////////////////////////////////////////////////////

const GTestRec gTestRecs[] = {
    { test_clear,            "clear"        },
    { test_hairlines_nodraw, "lines_nodraw" },
    { test_rect_nodraw,      "rect_nodraw"  },

    { test_poly_nodraw,        "poly_nodraw"            },
    { test_fatline_nodraw,     "test_fatline_nodraw"    },
    { test_fatline_degenerate, "test_fatline_degenerate"},

    { test_matrix,       "matrix_setters"    },
    { test_matrix_inv,   "matrix_inv"        },
    { test_matrix_map,   "matrix_map"        },
    { test_clamp_shader, "clamp_shader"      },


    { test_path,        "path",             },
    { test_path_rect,   "path_rect",        },
    { test_path_poly,   "test_path_poly",   },
    { test_path_transform, "path_transform" },
    { test_path_nodraw, "path_nodraw" },

    { test_edger_quads, "test_edger_quads"  },
    { test_path_oval, "test_path_oval"  },
    { test_path_transform2, "test_path_transform2" },
    { test_path_bounds, "path_bounds" },
    { test_modalpha, "mod_alpha_shader" },

    { nullptr, nullptr },
};

bool gTestSuite_Verbose;
bool gTestSuite_CrashOnFailure;
