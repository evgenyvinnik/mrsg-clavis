
/* Modified by Evgeny Vinnik (evinnik@sfu.ca) on July 25th, 2013 for the project MRSG-Clavis.*/

/* Copyright (c) 2012. MRSG Team. All rights reserved. */

/* This file is part of MRSG.

MRSG is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

MRSG is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with MRSG.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef DFS_H
#define DFS_H

/** @brief  Matrix that maps chunks to workers. */
char***  chunk_owners;

/**
 * @brief  Distribute chunks (and replicas) to DataNodes.
 */
void distribute_data (int configuration_id);

/**
 * @brief  Default data distribution algorithm.
 */
void default_dfs_f (char** dfs_matrix, size_t chunks, size_t workers, unsigned int replicas, int configuration_id);

/**
 * @brief  Choose a random DataNode that owns a specific chunk.
 * @param  cid  The chunk ID.
 * @return The ID of the DataNode.
 */
unsigned int find_random_chunk_owner (size_t cid, int configuration_id);

/**
 * @brief  DataNode main function.
 *
 * Process that listens for data requests.
 */
int data_node (int argc, char *argv[]);

#endif /* !DFS_H */
