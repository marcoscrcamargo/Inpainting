import imageio
import numpy as np

def inside(x, y):
	return 0 <= x and x < f.shape[0] and 0 <= y and y < f.shape[1]

RATIO = 0.02

print("Reading image...")

# Reading image.
f = imageio.imread("in.bmp")
f = f[:,:,:3]
mask = np.zeros((f.shape[0], f.shape[1]), np.uint8)
ans = np.empty(f.shape, np.uint8)
threshold = int(RATIO * (f.shape[0] * f.shape[1]))
freq = {}

print("Counting frequency...")

# Counting frequency.
for x in range(f.shape[0]):
	for y in range(f.shape[1]):
		rgb = (f[x][y][0], f[x][y][1], f[x][y][2])

		if rgb in freq:
			freq[rgb] += 1
		else:
			freq[rgb] = 1

print("Painting mask...")

# Painting.
for x in range(f.shape[0]):
	for y in range(f.shape[1]):
		rgb = (f[x][y][0], f[x][y][1], f[x][y][2])

		if freq[rgb] < threshold:
			mask[x][y] = 255

print("Processing image...")
imageio.imwrite("mask.bmp", mask)

k = 5
a = (k - 1) // 2

for bad_x in range(f.shape[0]):
	for bad_y in range(f.shape[1]):
		print("Inpainting pixel", (bad_x, bad_y))

		if mask[bad_x][bad_y] == 0:
			best_dist = -1

			# Retrieve most similar window.
			for x in range(f.shape[0]):
				for y in range(f.shape[1]):
					if mask[x][y] != 0:
						dist = 0
						used = 0

						# Calculate distance between window centered in (x, y) and window centered in (bad_x, bad_y)
						for i in range(-a, a + 1):
							for j in range(-a, a + 1):
								use = True

								# Retrieving a good pixel from a bad pixel area.
								if inside(bad_x + i, bad_y + j):
									if mask[bad_x + i][bad_y + j] == 0:
										use = False
									else:
										bad_p = f[bad_x + i, bad_y + j,:]
								else:
									bad_p = np.array([0, 0, 0])

								# Retrieving a good pixel from a good pixel area.
								if inside(x + i, y + j):
									if mask[x + i][y + j] == 0:
										use = False
									else:
										p = f[x + i, y + j,:]
								else:
									p = np.array([0, 0, 0])

								if use:
									used += 1
									dist += np.sqrt(((p - bad_p) ** 2).sum())

						dist /= used

						if best_dist == -1 or dist < best_dist:
							best_dist = dist
							best_p = f[x, y,:]

			ans[bad_x, bad_y,:] = best_p
		else:
			ans[bad_x, bad_y,:] = f[bad_x, bad_y,:]

print("Writing image...")
imageio.imwrite("out.bmp", ans)