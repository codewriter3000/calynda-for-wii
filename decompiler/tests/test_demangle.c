/*
 * Tests for the CodeWarrior demangler (requirement 5).
 */
#include "cli/demangle.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void check(const char *mangled, const char *expect_pretty) {
    CwDemangled d;
    int rc = cw_demangle(mangled, &d);
    if (!rc) {
        fprintf(stderr, "demangle failed: %s\n", mangled);
    }
    assert(rc == 1);
    if (strcmp(d.pretty, expect_pretty) != 0) {
        fprintf(stderr, "mismatch for %s:\n  got:  %s\n  want: %s\n",
                mangled, d.pretty, expect_pretty);
    }
    assert(strcmp(d.pretty, expect_pretty) == 0);
    printf("ok - %s -> %s\n", mangled, d.pretty);
}

int main(void) {
    /* 5.1 / 5.2 basic primitive + qualifier parsing. */
    check("foo__Fv",            "foo(void)");
    check("bar__FiPc",          "bar(int, char*)");
    check("baz__FfdUi",         "baz(float, double, unsigned int)");
    check("Read__3FooFv",       "Foo::Read(void)");

    /* Q<n> qualified namespaces. */
    check("adjust__Q23EGG7ExpHeapFv", "EGG::ExpHeap::adjust(void)");
    check("__ct__Q34nw4r2ut13DvdFileStreamFl",
          "nw4r::ut::DvdFileStream::DvdFileStream(long)");
    check("__dt__Q44nw4r2ut6detail12LinkListImplFv",
          "nw4r::ut::detail::LinkListImpl::~LinkListImpl(void)");

    /* Pointer-to-const. */
    check("__ct__Q34nw4r2ut13DvdFileStreamFPC11DVDFileInfob",
          "nw4r::ut::DvdFileStream::DvdFileStream(const DVDFileInfo*, bool)");

    /* 5.5 operator overloads. */
    check("__pl__3FooFRC3Foo", "Foo::operator+(const Foo&)");
    check("__eq__3FooFRC3Foo", "Foo::operator==(const Foo&)");
    check("__ml__3FooFf",      "Foo::operator*(float)");
    check("__vc__3FooFi",      "Foo::operator[](int)");

    {
        /* 5.4 kind classification. */
        CwDemangled d;
        assert(cw_demangle("__ct__3FooFv", &d));
        assert(d.kind == CW_KIND_CTOR);
        assert(strcmp(d.method, "Foo") == 0);
        assert(cw_demangle("__dt__3FooFv", &d));
        assert(d.kind == CW_KIND_DTOR);
        assert(strcmp(d.method, "~Foo") == 0);
        assert(cw_demangle("__pl__3FooFi", &d));
        assert(d.kind == CW_KIND_OPERATOR);
        assert(cw_demangle("foo__Fv", &d));
        assert(d.kind == CW_KIND_FREE);
        assert(cw_demangle("do_it__3FooFv", &d));
        assert(d.kind == CW_KIND_REGULAR);
        /* Not mangled: ok == 0, pretty == input verbatim. */
        assert(cw_demangle("memcpy", &d) == 0);
        assert(strcmp(d.pretty, "memcpy") == 0);
    }

    /* 5.3 namespace -> Calynda package mapping. */
    {
        char out[128];
        cw_to_calynda_path("EGG::ExpHeap", out, sizeof out);
        assert(strcmp(out, "egg.ExpHeap") == 0);
        cw_to_calynda_path("nw4r::ut::DvdFileStream", out, sizeof out);
        assert(strcmp(out, "nw4r.ut.DvdFileStream") == 0);
        cw_to_calynda_path("JSystem::Foo::Bar", out, sizeof out);
        assert(strcmp(out, "jsystem.Foo.Bar") == 0);
        printf("ok - namespace -> package\n");
    }

    /* 5.4 / 5.5 binding. */
    {
        CwDemangled d;
        char out[160];

        cw_demangle("__ct__Q23EGG7ExpHeapFv", &d);
        cw_to_calynda_binding(&d, out, sizeof out);
        assert(strcmp(out, "egg.ExpHeap.new") == 0);

        cw_demangle("__dt__Q23EGG7ExpHeapFv", &d);
        cw_to_calynda_binding(&d, out, sizeof out);
        assert(strcmp(out, "egg.ExpHeap.drop") == 0);

        cw_demangle("__pl__3FooFRC3Foo", &d);
        cw_to_calynda_binding(&d, out, sizeof out);
        assert(strcmp(out, "foo.plus") == 0);

        cw_demangle("adjust__Q23EGG7ExpHeapFv", &d);
        cw_to_calynda_binding(&d, out, sizeof out);
        assert(strcmp(out, "egg.ExpHeap.adjust") == 0);
        printf("ok - binding mapping\n");
    }

    /* 5.6 uniqueness. */
    {
        CwRenameTable *t = cw_rename_new();
        char a[64], b[64], c[64];
        int r0 = cw_rename_unique(t, "egg.Heap.alloc", a, sizeof a);
        int r1 = cw_rename_unique(t, "egg.Heap.alloc", b, sizeof b);
        int r2 = cw_rename_unique(t, "egg.Heap.alloc", c, sizeof c);
        assert(r0 == 0 && strcmp(a, "egg.Heap.alloc") == 0);
        assert(r1 == 1 && strcmp(b, "egg.Heap.alloc_2") == 0);
        assert(r2 == 1 && strcmp(c, "egg.Heap.alloc_3") == 0);
        cw_rename_free(t);
        printf("ok - uniqueness suffixing\n");
    }

    printf("-- all demangle tests passed --\n");
    return 0;
}
