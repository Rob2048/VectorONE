
uint8_t _frameBuffer[1024 * 704];

typedef struct 
{
	float minX;
	float minY;
	float maxX;
	float maxY;
	float cX;
	float cY;

} blob;

int 	blobCount = 0;
blob 	blobs[256];

float distSq(float X1, float Y1, float X2, float Y2)
{
	return (X2 - X1) * (X2 - X1) + (Y2 - Y1) * (Y2 - Y1);
}

void ProcessPixels(void)
{
	int width = 1024;
	int height = 704;
	//int pixelCount = width * height;

	int brightPixels = 0;

	blobCount = 0;

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			//if (buffer[y * width + x] > 95 && buffer[y * width + x] < 189)
			if (_frameBuffer[y * width + x] > 60)
			{
				++brightPixels;

				bool blobFound = false;
				
				// Could check all blobs and find closest.

				for (int b = 0; b < blobCount; ++b)
				{
					blob* cb = blobs + b;

					float cx = (cb->minX + cb->maxX) / 2;
					float cy = (cb->minY + cb->maxY) / 2;
					float d = distSq(x, y, cx, cy);

					if (d < 8 * 8)
					{
						if (x < cb->minX) cb->minX = x;
						if (y < cb->minY) cb->minY = y;
						if (x > cb->maxX) cb->maxX = x;
						if (y > cb->maxY) cb->maxY = y;

						blobFound = true;
						break;
					}
				}

				if (!blobFound)
				{
					blobs[blobCount].minX = x;
					blobs[blobCount].minY = y;
					blobs[blobCount].maxX = x;
					blobs[blobCount].maxY = y;

					++blobCount;
				}
			}
		}
	}

	for (int i = 0; i < blobCount; ++i)
	{
		blob* b = blobs + i;

		b->cX = 0.0f;
		b->cY = 0.0f;

		float totalWeight = 0.0f;
		// Count total weight
		for (int y = b->minY; y < b->maxY + 1; ++y)
		{
			for (int x = b->minX; x < b->maxX + 1; ++x)
			{
				uint8_t p = _frameBuffer[y * width + x];
				if (p > 60)
				{
					totalWeight += p;
				}
			}
		}

		for (int y = blobs[i].minY; y < blobs[i].maxY + 1; ++y)
		{
			for (int x = blobs[i].minX; x < blobs[i].maxX + 1; ++x)
			{
				uint8_t p = _frameBuffer[y * width + x];
				if (p > 60)
				{
					float pixelV = p;

					b->cX += x * (pixelV / totalWeight);
					b->cY += y * (pixelV / totalWeight);
				}
			}
		}
	}

	//printf("%d %d %d %d\n", brightPixels, blobCount, (int)t1, (int)startTime);
}