Computer-Human Interaction Research

The International Telecommunication Union conducted extensive research to develop a comprehensive quantitative psychological model of telephone user satisfaction. The E-model accounts for sources of noise, delay, and other transmission impairments, and it accounts for contexts that affect user expectations. It computes a psychological rating score from these measurements and inputs, and predicts how satisfied users will be with the characterized experiences.

There has been a lot of research into computer user satisfaction, but it seems to have mostly produced punchy take-home lessons and rules-of-thumb, as opposed to a quantitative psychological model like the ITU E-model.

The Chrome Performance Insights team is looking for such a quantitative psychological model of computer users so that we can identify and rank performance issues.

This is a simple web app that asks the user to perform interactions such as clicking, scrolling, or loading.
Those interactions are artificially delayed by a random amount of time.
Users are asked if the delay was "too slow" or "fast enough".
A histogram is presented of the percentage of "fast enough" responses for each bucket of delay.
