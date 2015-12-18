It's 2015, and I still can't engineer without killing trees. Maybe it's just me, but I find that I cannot think without being able to write equations and draw graphs and diagrams all on the same piece of paper. Once I've figured everything out, then I can translate it to a digital format, or, rather, a mix of about 4 digital formats depending on the project.

I should be able to type a text format, and see equations and diagrams all on the same screen. And since it's a computer, it should be able to simulate what would happen if a diagram were built in the real world. And it must be web-based.

[Markdown+KaTeX](https://github.com/abejfehr/markdown-preview-katex) looks amazing!
But there's no interactivity. In order to really understand an equation, I need to see it in action and play with it.

Also, throughout the course of a project, I change designs often, sometimes exploring multiple designs at once. If I could interactively change something and see how it affects other parts of the design, then I could iterate on designs a lot faster.
See also openjscad, circuitjs, desmos, codebender. (I wish there were an AVR/PIC simulator online. Maybe using an [online C++ compiler](tutorialspoint.com/compile_cpp_online.php)?)

Can we define Markdown or KaTeX macros to generate interactive widgets?

Let's start simple. A textbox to define a variable:

\?{R1=10k}

Units are important. Let's track them and type-check them. R1 is probably a resistor. Maybe we can infer units from variable names?

R: ohm;
C: farad;
V: volt;
L: henry;
I: amp;
W: watt;
Hz: Hz;
S: second;
M: meter;
G: grams;
otherwise: dimensionless, or inferred dimensions, or a way to explicitly.

Now, we can supercharge existing KaTeX equations such that, if all of their variables are defined, we can go ahead and solve them interactively.

$ Hz1 = 1 / (tau * R1 * C1) \=? $ would display the equation normally, but then also display " = 20Hz", and define the "Hz1" variable for other equations.

If we have variables, then we've implicitly defined a namespace. Let's define a macro to make them explicit, so that a single document can contain two different designs with two different "R1" variables.

\namespace{foo}

Let's avoid nesting namespaces to keep things simple, so a single macro can both end the previous namespace and begin a new one.

How can we embed more powerful tools such as desmos, circuitjs, openjscad, codebender, etc?

Those tools use their own kinds of documents: equations and settings for desmos; netlists for circuitjs; javascript for openjscad.

Those documents should be versioned with our markdown+katex document. Speaking of versioning, integration with github or google docs, natch.

The integration should be bi-directional. Changing a circuit diagram should update our uber-document, and vice-versa.

The embedding mechanism should allow sharing the namespace so that changing R1 in our markdown+katex document changes R1 in circuitjs. openjscad already has a concept of settings.

Some of those tools allow loading a document from a URL, but that would require a server to pick apart our uber-document and deliver a part of the document.

Maybe we could get them all to accept messages from window.postMessage()? And send messages back when the user updates the document from within the embedded widget?

