#define CL_CEARCH_IMPLEMENTATION

#include <stdio.h>
#include "./cearch.h"

#define quit(code) do {return_code = code; goto defer;} while(0)

int main(void) {
    int return_code = 0;

    CL_Cearch cearch = {0};

    cl_cearch_set_boolean_variable(&cearch, "is_reminder", true);
    cl_cearch_set_boolean_variable(&cearch, "is_reminder", false);
    cl_cearch_set_single_value_atom_variable(&cearch, "state", "todo");
    cl_cearch_set_single_value_atom_variable(&cearch, "state", "doing");
    cl_cearch_set_single_value_atom_variable(&cearch, "name", "hello world");
    cl_cearch_set_multiple_values_atom_variable(&cearch, "tags", "high_priority");
    cl_cearch_set_multiple_values_atom_variable(&cearch, "tags", "wodo_project");
    cl_cearch_set_multiple_values_atom_variable(&cearch, "tags", "high_priority");
    cl_cearch_set_multiple_values_atom_variable(&cearch, "reminders", "testing");

    printf("Boolean vars:\n");
    for (int i = 0; i < cearch.boolean_variables_count; ++i) {
        printf("  %s: %s\n", cearch.boolean_variables[i].name, cearch.boolean_variables[i].value ? "true" : "false");
    }

    printf("\n");

    printf("Single Atom Variables:\n");
    for (int i = 0; i < cearch.single_value_atom_variables_count; ++i) {
        printf("  %s: %s\n", cearch.single_value_atom_variables[i].name, cearch.single_value_atom_variables[i].value);
    }

    printf("\n");

    printf("Multiple Values Atom Variables:\n");
    for (int i = 0; i < cearch.multiple_values_atom_variables_count; ++i) {
        printf("  %s:\n", cearch.multiple_values_atom_variables[i].name);
        for (int j = 0; j < cearch.multiple_values_atom_variables[i].values_count; j++) {
            printf("    %s\n", cearch.multiple_values_atom_variables[i].value[j]);
        }
    }

    if (!cl_cearch_compile(&cearch,  "(state not in [:blocked, :done] and is_reminder) or tags contains :high_priority")) {
        cl_cearch_dump_tokens(&cearch);

        quit(1);
    }

    if (cl_cearch_match(&cearch)) {
        printf("matched\n");
    } else {
        printf("did not match\n");
    }

defer:
    cl_cearch_free(&cearch);

    return return_code;
}
