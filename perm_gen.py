#

neighbors = (
    (1, 4), (0, 2, 5), (1, 3, 6), (2, 7),
    (0, 5), (4, 6, 1), (5, 7, 2), (6, 3)
)

perms = []

for i in range(8):
    for j in neighbors[i]:
        for k in neighbors[j]:
            if i == k: continue
            for l in neighbors[k]:
                # ij, jk, kl, ik, jl, il
                if j == l or i == l: continue
                perms.append((i, j, k, l))

def compact(x):
    return f'0x{x[0]}{x[1]}{x[2]}{x[3]}'

print(', '.join(compact(p) for p in perms))