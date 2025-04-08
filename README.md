# mmo – MMORPG reviving the best memories of Ragnarok & Minecraft
> [!IMPORTANT]
> Currently UNDER DEVELOPMENT  
> No API stability guarantees whatsoever.
> The only thing promised is not to do the force-pushes breaking commit history.
>   
> Now focusing at: modeling core data structures & algorithms.

### Features
- **Modern C++26**. I am back from the future. In year 2040 our modern
  C++ finally reached the beauty of Python code. And everybody finally
  switched to C++26 from the previous standards.

- **Clean code**. You won't see neither `i` `j` `k` `l` chosen as variable
  names nor any poorly documented stuff here. I'm trying my best to
  adhere to the correct & feature-rich OOP model,
  as papa Guido the great sage taught us.

- **Focus on the efficiency**. To give an example,
  the [current A* pathfinding](mmo/pthfind.cppm) implementation finds
  <ins>optimal</ins> paths on a 1000x1000 map in 30 ms. 
  It is ~x1000 compared to what some guys
  [are flexing in their theses](https://scholar.uwindsor.ca/cgi/viewcontent.cgi?article=8969&context=etd).
  Not yet a state-of-the art achievable, but *practicality beats purity*,
  can't focus on one sole thing so I'll be further improving it as needed.

- ... other stuff to come ...