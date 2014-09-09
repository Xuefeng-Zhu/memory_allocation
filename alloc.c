/** @file alloc.c */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>


/** Structure for each dictonary entry. */
typedef struct _dictionary_entry_t 
{
    size_t size;
	struct _dictionary_entry_t *prev;
    struct _dictionary_entry_t *next;
} dictionary_entry_t;

//structure for allocated block
typedef struct _allocate_block 
{
    size_t size;
} allocate_block;

//structure for footer in the block
typedef struct _footer 
{
	struct _dictionary_entry_t *prev;
} footer;

dictionary_entry_t *dictionary = NULL;
//inital heap position
void* heapStart = NULL;


//set footer function
inline void setfooter(dictionary_entry_t *ptr)
{
	footer *temp = (void*)ptr + sizeof(allocate_block) + ptr->size;
	temp->prev = ptr;
}

//add ptr into the free list
inline void addPtr(dictionary_entry_t* ptr)
{
	ptr->next = dictionary;
	if (dictionary != NULL)
		dictionary->prev = ptr;
	ptr->size += 1;
	dictionary = ptr;
}

//delte ptr from the free list 
inline void deletePtr(dictionary_entry_t *ptr)
{

	if (ptr == dictionary)
	{
		dictionary = ptr->next;
	}
	else 
	{
		ptr->prev->next = ptr->next;
		if (ptr->next != NULL)
			ptr->next->prev = ptr->prev;
	}

	ptr->size -= 1;
}

inline dictionary_entry_t* coalesing_free(dictionary_entry_t *curr)
{
	void* temp = (void*)curr + sizeof(allocate_block) + curr->size + sizeof(footer);
	
	if (temp < sbrk(0))
	{
		dictionary_entry_t *nextPtr = temp;
		if ((nextPtr->size & 1) == 1)
		{
			deletePtr(nextPtr);
			curr->size += nextPtr->size + sizeof(allocate_block) + sizeof(footer);	
			setfooter(curr);
		}
	}
	
	footer *ftr = (void*)curr - sizeof(footer);
	if ((void*)ftr > heapStart)
	{
		dictionary_entry_t *prevPtr = ftr->prev;
		if ((prevPtr->size & 1) == 1)
		{
			deletePtr(prevPtr);
			prevPtr->size += curr->size + sizeof(allocate_block) + sizeof(footer);
			setfooter(prevPtr);
			curr = prevPtr;
		}
	}
	return curr;
}

inline dictionary_entry_t* coalesing_relloc(dictionary_entry_t *curr, size_t size)
{
	void* temp = (void*)curr + sizeof(allocate_block) + curr->size + sizeof(footer);
	size_t currSize = curr->size;	
	if (temp < sbrk(0))
	{
		dictionary_entry_t *nextPtr = temp;
		if ((nextPtr->size & 1) == 1)
		{
			deletePtr(nextPtr);
			curr->size += nextPtr->size + sizeof(allocate_block) + sizeof(footer);	
			setfooter(curr);
		}
	}

	if (curr->size > size)
	{
		return curr;	
	}

	footer *ftr = (void*)curr - sizeof(footer);
	if ((void*)ftr > heapStart)
	{
		dictionary_entry_t *prevPtr = ftr->prev;
		if ((prevPtr->size & 1) == 1)
		{
			if (prevPtr->size + curr->size + sizeof(footer) + sizeof(allocate_block) - size < 50)
			{

				deletePtr(prevPtr);
				prevPtr->size += curr->size + sizeof(allocate_block) + sizeof(footer);
				setfooter(prevPtr);
				memmove((void *)prevPtr + sizeof(allocate_block), (void*)curr + sizeof(allocate_block), currSize);
				curr = prevPtr;
			}
		}
	}
	return curr;
}

inline void cutMemory(dictionary_entry_t *ptr, size_t size)
{
	int new_size = ptr->size - (size + sizeof(dictionary_entry_t) + sizeof(footer));
	if (new_size  > 0.3 * ptr->size)
	{
		dictionary_entry_t *new_entry = (void*)ptr + sizeof(allocate_block) + size + sizeof(footer);	
		new_entry->size = new_size + 2 * sizeof(dictionary_entry_t*);
		setfooter(new_entry);
		addPtr(new_entry);
		ptr->size = size;
		setfooter(ptr);
	}
	
}
/**
 * Allocate space for array in memory
 * 
 * Allocates a block of memory for an array of num elements, each of them size
 * bytes long, and initializes all its bits to zero. The effective result is
 * the allocation of an zero-initialized memory block of (num * size) bytes.
 * 
 * @param num
 *    Number of elements to be allocated.
 * @param size
 *    Size of elements.
 *
 * @return
 *    A pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory, a
 *    NULL pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
 */
void *calloc(size_t num, size_t size) 
{
    /* Note: This function is complete. You do not need to modify it. */
    void *ptr = malloc(num * size);
	
    if (ptr) 
	{
        memset(ptr, 0x00, num * size);
    }

    return ptr;
}


/**
 * Allocate memory block
 *
 * Allocates a block of size bytes of memory, returning a pointer to the
 * beginning of the block.  The content of the newly allocated block of
 * memory is not initialized, remaining with indeterminate values.
 *
 * @param size
 *    Size of the memory block, in bytes.
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a null pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
 */
void *malloc(size_t size) 
{
    if (heapStart == NULL)
	{
		heapStart = sbrk(0);	
	}
	
    /* See if we have free space of enough size. */
    dictionary_entry_t *p = dictionary;
    dictionary_entry_t *chosen = NULL;

	if (size < 2 * sizeof(dictionary_entry_t*))
	{
		size = 2 * sizeof(dictionary_entry_t*);
	}
	else
	{
		size += size % 2;	
	}

    while (p != NULL) 
	{
        if ( p->size >= size) 
		{
			if (chosen == NULL || (chosen && p->size < chosen->size)) 
			{
				chosen = p;
			}
        }
        p = p->next;
    }
    
    if (chosen) 
	{
		deletePtr(chosen);
		cutMemory(chosen, size);
        return (void*)chosen + sizeof(allocate_block);
    }




    /* Add our entry to the dictionary */
    allocate_block * newBlock = sbrk(sizeof(allocate_block) + size + sizeof(footer));
	if (newBlock == (void*)-1)
	{
		return NULL;	
	}
    newBlock->size = size;
	setfooter((dictionary_entry_t*)newBlock);

    return (void*)(newBlock+1);
}


/**
 * Deallocate space in memory
 * 
 * A block of memory previously allocated using a call to malloc(),
 * calloc() or realloc() is deallocated, making it available again for
 * further allocations.
 *
 * Notice that this function leaves the value of ptr unchanged, hence
 * it still points to the same (now invalid) location, and not to the
 * null pointer.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(),
 *    calloc() or realloc() to be deallocated.  If a null pointer is
 *    passed as argument, no action occurs.
 */
void free(void *ptr) 
{
    // "If a null pointer is passed as argument, no action occurs."
    if (!ptr)
        return;

    /* Free the memory in our dictionary. */
    dictionary_entry_t *p = ptr - sizeof(allocate_block);

	p = coalesing_free(p);

	addPtr(p);
    return;
}


/**
 * Reallocate memory block
 *
 * The size of the memory block pointed to by the ptr parameter is changed
 * to the size bytes, expanding or reducing the amount of memory available
 * in the block.
 *
 * The function may move the memory block to a new location, in which case
 * the new location is returned. The content of the memory block is preserved
 * up to the lesser of the new and old sizes, even if the block is moved. If
 * the new size is larger, the value of the newly allocated portion is
 * indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning
 * a new block of size bytes and returning a pointer to the beginning of it.
 *
 * In case that the size is 0, the memory previously allocated in ptr is
 * deallocated as if a call to free was made, and a NULL pointer is returned.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(), calloc()
 *    or realloc() to be reallocated.
 *
 *    If this is NULL, a new block is allocated and a pointer to it is
 *    returned by the function.
 *
 * @param size
 *    New size for the memory block, in bytes.
 *
 *    If it is 0 and ptr points to an existing block of memory, the memory
 *    block pointed by ptr is deallocated and a NULL pointer is returned.
 *
 * @return
 *    A pointer to the reallocated memory block, which may be either the
 *    same as the ptr argument or a new location.
 *
 *    The type of this pointer is void*, which can be cast to the desired
 *    type of data pointer in order to be dereferenceable.
 *    
 *    If the function failed to allocate the requested block of memory,
 *    a NULL pointer is returned, and the memory block pointed to by
 *    argument ptr is left unchanged.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
 */
void *realloc(void *ptr, size_t size)
{
    // "In case that ptr is NULL, the function behaves exactly as malloc()"
    if (!ptr)
	{
        return malloc(size);
    }

    // "In case that the size is 0, the memory previously allocated in ptr
    //  is deallocated as if a call to free() was made, and a NULL pointer
    //  is returned."
    if (!size)
	{
        free(ptr);
        return NULL;
    }

    dictionary_entry_t *entry = ptr - sizeof(allocate_block);
 	size += size%2;
	if ((void*)entry + sizeof(allocate_block) + entry->size + sizeof(footer) == sbrk(0))
	{

		if (size <= entry->size)
		{
			sbrk(-entry->size+size);	

		}
		else 
		{
			sbrk(size - entry->size);
		}
		entry->size = size;
		setfooter(entry);
		return (void*)entry + sizeof(allocate_block);
	} 
 	size_t entrySize = entry->size;
 	if (entry->size < size)
	{
		entry = coalesing_relloc(entry, size);
	}

    if(size <= entry->size)
	{
		cutMemory(entry, size);
	    return (void*)entry + sizeof(allocate_block);
	}
    else
	{
		void* newPtr = malloc(size);
	    memcpy(newPtr, (void*)entry + sizeof(allocate_block), entrySize);
		free((void*) entry + sizeof(allocate_block));
		return newPtr;
    }
}



