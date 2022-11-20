import random
import statistics

probeWeight = random.sample(range(0, 320), 32)
probeMin = min(probeWeight)
probeMax = max(probeWeight)

probeMean = statistics.fmean(probeWeight)

RAYBUDGET = 2 ** 17

probeBudget = [0]*32

for i in range(32):
    probeBudget[i] = int((((probeWeight[i] - probeMin) * (RAYBUDGET)) / (probeMax - probeMin)))

print(probeBudget)
print(sum(probeBudget))
print(RAYBUDGET)
