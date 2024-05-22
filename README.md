par
===

A Github mirror of the [par][par] original [repository][repository] by
[Adam M. Costello][amc].

Originally written in 1993, aiming to do one task:

reformat a single paragraph that might have a border on either side.

Description
===========

par is a paragraph reformatter, vaguely similar to fmt, but better.

For example, the command “par 44gqr”, given the input:

        John Q. Public writes:
        > Jane Doe writes:
        > >
        > > May I remind people that this newsgroup
        > > is for posting binaries only.  Please keep
        > > all discussion in .d where it belongs.
        > Who appointed you net.god?
        > I'll discuss things here if I feel like it.
        Could you two please take this to e-mail?

        **********************************************
        ** Main's Law: For every action there is an **
        ** equal and opposite government program.   **
        **********************************************

Would produce the output:

        John Q. Public writes:

        > Jane Doe writes:
        >
        > > May I remind people that this
        > > newsgroup is for posting
        > > binaries only.  Please keep
        > > all discussion in .d where it
        > > belongs.
        >
        > Who appointed you net.god?  I'll
        > discuss things here if I feel like
        > it.

        Could you two please take this to
        e-mail?

        ************************************
        ** Main's Law: For every action   **
        ** there is an equal and opposite **
        ** government program.            **
        ************************************

Building
=========

On MacOS/Linux


        make -f protoMakefile CC="cc -c" LINK1="cc" LINK2="-o" RM="rm" JUNK="" $*


Copyright
==========

Copyright belongs to the original authors even though this is a mirror.

Copyright 1993, 1996, 2000, 2001, 2020 Adam M. Costello

[par]: http://www.nicemice.net/par/
[repository]: https://bitbucket.org/amc-nicemice/
[amc]: http://www.nicemice.net/amc/
