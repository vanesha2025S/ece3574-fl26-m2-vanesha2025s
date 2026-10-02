# M2 Technical & Design Understanding

This file is an assessed technical-understanding artifact, not ordinary project documentation.
Answer all four questions using your own submitted implementation. Concise answers are acceptable when they are technically correct and specific.

Generic descriptions of C++ concepts or restatements of the assignment that do not identify and explain corresponding parts of your code will receive limited credit.

## 1. Polymorphism and dynamic dispatch - 1.5 points

One place runtime polymorphism happens in my code is in ProcessingCore::search(). The base interface is RetrievalStrategy, and the default implementation is RetrievalEngine. In ProcessingCore::Impl, the retrieval strategy is stored as a std::unique_ptr<RetrievalStrategy>. When search() runs, it calls impl_->retrieval->search(query, k, impl_->chunks, impl_->index). Since search() is virtual, the program uses whichever retrieval strategy was given to ProcessingCore. Normally this is RetrievalEngine, but in my tests I use FirstOnly, which returns a score of 99.0. This shows that my custom strategy is actually being called. If search() was not virtual, C++ would not choose the derived version at runtime, so FirstOnly would not be called this way.

## 2. Ownership and lifetime - 1.5 points

In my default constructor, the strategies are created using std::make_unique, like std::make_unique<RetrievalEngine>(). They are passed into the ProcessingCore constructor, checked to make sure they are not null, and then moved into impl_. ProcessingCore::Impl then owns the three strategies using unique_ptrs. This means each strategy only has one owner and is automatically destroyed when ProcessingCore is destroyed.

ProcessingCore is move-only because the strategies have one owner, so they should be moved instead of copied. Moving transfers ownership of them to the new ProcessingCore. The strategy interfaces also need virtual destructors because objects like RetrievalEngine are stored using pointers to their base interfaces. This makes sure the correct derived object is destroyed when it is no longer needed.

## 3. Architecture, extensibility, and M1 compatibility - 1.5 points

One change I made for M2 was having ProcessingCore store the three strategy interfaces instead of depending directly on one implementation. The default constructor still creates Chunker, RetrievalEngine, and ContextBuilder, so using ProcessingCore normally keeps the same behavior from M1. I can also pass in different strategies through the other constructor without changing how rebuild(), search(), or build_context() are called. Another way to design this would be to add separate functions to ProcessingCore for each type of chunking, retrieval, or context behavior. The strategy design is better for M2 because I can add or switch implementations without having to keep changing ProcessingCore or its normal API.

## 4. Testing and defect reasoning - 1.5 points 

One test I added uses my custom FirstOnly retrieval strategy. FirstOnly always returns the first chunk with a score of 99.0. After creating ProcessingCore with my custom strategies, I call search() and check that the result has a score of 99.0. This tests that ProcessingCore actually uses the retrieval strategy that was passed into it at runtime. This test could catch a bug where ProcessingCore accepts a custom strategy but still uses RetrievalEngine inside search(). The score of 99.0 makes the custom behavior easy to see, since that value comes directly from FirstOnly. This goes beyond the public tests because I'm specifically testing my own custom strategy through ProcessingCore instead of only checking the default behavior.
