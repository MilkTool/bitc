[The BitC Programming Language](http://bitc-lang.org/)

Download:  http://bitc-lang.org/download/

Update from main Mercurial repo:

    git-hg clone http://dev.eros-os.com/hg/bitc

    or

    git-hg pull http://dev.eros-os.com/hg/bitc

Build:

    cd bitc/src
    # Set CXXFLAGS so that you can help us debug the compiler:
    CXXFLAGS=-g ./configure
    make

If you are trying to track the development updates as we go, build with make clean:

    cd bitc/src
    # Set CXXFLAGS so that you can help us debug the compiler:
    CXXFLAGS=-g ./configure
    make clean
    make



