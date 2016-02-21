/****************************************************/
/* File: QSORT.c                                    */
/* a verson of quick sort                           */
/****************************************************/

int partition( int* a, int p, int r )
{
  int x, i, j;

  x = a[r];
  i = p - 1;
  for( j = p; j < r; j += 1 )
  {
    if( a[j] <= x )
    {
      i += 1;
      swap( &a, &a[j] );
    }
  }
  swap( &a[i+1], &a[r] );
  return i + 1;
}

void qsort( int *a, int p, int r)
{
  int q;

  if( p < r )
  {
    q = partition( a, p, r );
    qsort( a, p, q-1 );
    qsort( a, q+1, r );
  }
}

void main()
{
  int a[5];
  int i;

  for( i = 0; i < 5; i += 1 )
    scanf( "%d", &a );

  qsort( a, 0, 4 );

  printf( "-----------\n" );
  for( i = 0; i < 5; i += 1 )
    printf( "%d\n", a );
}

void swap( int *a, int *b )
{
  int t;

  t = *a; *a = *b; *b = t;
}