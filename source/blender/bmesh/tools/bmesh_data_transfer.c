/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): Walid Shouman
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include "MEM_guardedalloc.h"

#include "BLI_utildefines.h"

#include "bmesh.h"

#include "DNA_meshdata_types.h"
#include "BKE_editmesh.h"

#include "BKE_editmesh_bvh.h"		//to use bvh
#include "bmesh_data_transfer.h"	//own include
#include "BKE_customdata.h"			//Custom Data related
#include "BLI_array.h"				//used for interpolation (reserving custom size)
#include "DNA_key_types.h"			//used for dealing with keyblocks
#include "BLI_math_vector.h"		//for copying vectors
#include "BLI_linklist.h"			//used for path finding (ie for seams)
#include "../editors/include/ED_mesh.h"		//used to check the editmesh in the flag_islands function
#include "BKE_mesh.h"				//used for UvVertMap in flag_islands function

#include "DNA_object_types.h"			//using the bDeformGroup
#include "BKE_bvhutils.h"				//using the bvhutils.h
#include "BKE_deform.h"

//---------------Declarations------------------------------

//---------------helping functions declarations -----------
static BMLoop* BM_vert_find_best_tan_match_loop(BMVert *v_src, BMLoop *l_dst);
static int BM_iter_loops_as_array(BMesh *bm, BMElem **array, int len);
static void BM_get_face_coord_list(float (**r_co_list)[3], BMFace *f);
static void BM_get_face_loops_as_indices(BMFace *f, int **r_loop_array, int *r_loop_array_len);
static void BM_get_face_verts_as_indices(BMFace *f, int **r_vert_array, int *r_vert_array_len);
static void append_index_list(int **out_list, const int out_list_len, int *in_list, const int in_list_len);
static void append_co_list(float (**out_list)[3], const int out_list_len, float (*in_list)[3], const int in_list_len);
#if 0
static void BM_get_face_elems_as_indices(BMFace *f, int *r_elem_array, int *r_elem_array_len, int htype);
static BMLoop* BM_face_find_best_tan_match_loop(BMFace *f_src, BMLoop *l_dst);
#endif

static int *BM_transform_index_mapping(BMesh *bm_src, BMElem **array_dst, int array_dst_count, int *index_mapping_in,
                                       int htype_from, int htype_to);
static bool BM_transform_index_multi_mapping(BMesh *bm_src, BMElem **array_dst, int array_dst_count, int *index_mapping_in,
                                      int htype_from, int htype_to, int ***r_index_mapping_out,
                                      int **r_index_mapping_out_len, float ***r_index_mapping_out_weights);
static void *BM_mesh_mapping(BMesh *bm_src, BMesh *bm_dst, const char htype);

//--------------index transfer declarations ----------
static void BM_mesh_cd_transfer_array(CustomData *cd_src, BMElem **array_src, int array_src_count,
                                       CustomData *cd_dst, BMElem **array_dst, int array_dst_count,
                                       const int layer_type, const struct ReplaceLayerInfo *replace_info);

static void BM_mesh_transfer_aligned(BMesh *bm_src, BMesh *bm_dst, const char htype, const int layer_type,
                                     const struct ReplaceLayerInfo *replace_info);

//--------------topology transfer declarations ------
static void BM_mesh_cd_transfer_mapped(CustomData *cd_src, BMElem **array_src, int array_src_count,
                                       CustomData *cd_dst, BMElem **array_dst, int array_dst_count,
                                       const int layer_type, const struct ReplaceLayerInfo *replace_info,
                                       int *index_mapping);

static void BM_mesh_transfer_mapped(BMesh *bm_src, BMesh *bm_dst, const char htype, const int layer_type,
                                    const struct ReplaceLayerInfo *replace_info);

static void set_loop_indices(BMesh *bm);

//----------interpolation transfer declarations--------
static void BM_mesh_cd_transfer_interpolated(CustomData *cd_src, BMElem **array_src, int array_src_count,
                                             CustomData *cd_dst, BMElem **array_dst, int array_dst_count,
                                             const int layer_type, const struct ReplaceLayerInfo *replace_info,
                                             int **index_mapping, int *index_mapping_count, float **index_mapping_weights);

static void BM_mesh_transfer_interpolated(BMesh *bm_src, BMesh *bm_dst, const char htype, const int layer_type,
                                    const struct ReplaceLayerInfo *replace_info);



//---------------Definitions---------------------------

bool BM_mesh_data_copy(BMesh *bm_src, BMesh* bm_dst, const struct ReplaceLayerInfo *replace_info, int type,
                       TransferMode transfer_mode, bool UNUSED(relative_to_target), float UNUSED(tmp_mat[4][4]),
                       bool UNUSED(use_tolerance), float UNUSED(tolerance))
{

//+-------------+
//|				|
//|	Block 1		|	//get a tree for the source (using BKE_bmbvh_new)
//|				|
//+-------------+



//+-------------+
//|				|
//|	Block 2		|	//make any needed initial allocations
//|				|
//+-------------+




//+-------------+
//|				|
//|	Block 3		|	//loop over each destination face
//|	Big-one		|
//+-------------+

	//transfer by indices	(3.0)
	//		|
	//		|
	//		V

	if(transfer_mode == TRANSFER_BY_INDEX) {
		switch (type) {
			case CD_SHAPEKEY:
			case CD_MDEFORMVERT:
				BM_mesh_transfer_aligned(bm_src, bm_dst, BM_VERT, type, replace_info);
				break;

			case CD_MLOOPCOL:
			case CD_MLOOPUV:
				BM_mesh_transfer_aligned(bm_src, bm_dst, BM_LOOP, type, replace_info);
		}
	}

	else if(transfer_mode == TRANSFER_BY_TOPOLOGY) {
		switch (type) {
			case CD_SHAPEKEY:
			case CD_MDEFORMVERT:
				BM_mesh_transfer_mapped(bm_src, bm_dst, BM_VERT, type, replace_info);
				break;

			case CD_MLOOPCOL:
			case CD_MLOOPUV:
				BM_mesh_transfer_mapped(bm_src, bm_dst, BM_LOOP, type, replace_info);
		}
	}

	else if (transfer_mode == TRANSFER_BY_INTERPOLATION) {
		switch (type) {
			case CD_SHAPEKEY:
			case CD_MDEFORMVERT:
				BM_mesh_transfer_interpolated(bm_src, bm_dst, BM_VERT, type, replace_info);
				break;

			case CD_MLOOPCOL:
			case CD_MLOOPUV:
				BM_mesh_transfer_interpolated(bm_src, bm_dst, BM_LOOP, type, replace_info);
				break;
		}
	}

	//transfer by interpolation	(3.1)
	//		|
	//		|
	//		V


	//+-----------------+
	//|	sub_Block 3.1	|	//get the best matching source face (using BKE_bmbvh_find_face_closest)
	//+-----------------+


			//we got a face			(3.1.a)
			//		|
			//		|
			//		V

		//+---------------------+
		//|	sub_Block 3.1.a.1	|	//updating the lookup tables
		//+---------------------+

		//+---------------------+
		//|	sub_Block 3.1.a.2	|	//Interpolate and get the weights
		//+---------------------+

		//+---------------------+
		//|	sub_Block 3.1.a.3	|	//Call the respective transfer function for all the layers with the transfer weights and any optional look up tables
		//+---------------------+


			//we didn't get a face	(3.1.b)
			//		|
			//		|
			//		V

		//+---------------------+
		//|	sub_Block 3.1.b.0	|	//update the lookup tables
		//+---------------------+

	return true;
}

//--------------index transfer definitions---------

/**
 * @brief BM_mesh_cd_transfer_mapped
 * Given source elements -@param array_src-
 *		and dest elements -@param array_dst-, copy the CD of type @param layer_type from @param cd_src to @param cd_dst
 * Layers to be copied must be given as the @param replace_info
 * Each elem in @param array_dst corresponds to the elem in @param array_src that has the same index
 *
 * @param cd_src			source cd layer
 * @param array_src			array of source elements
 * @param array_src_count	number of provided source elements
 * @param cd_dst			destination cd layer
 * @param array_dst			array of destination elements
 * @param array_dst_count	number of provided destination elements
 * @param layer_type		CD layer type
 * @param replace_info		layers to replace
 */

static void BM_mesh_cd_transfer_array(CustomData *cd_src, BMElem **array_src, int array_src_count,
                                      CustomData *cd_dst, BMElem **array_dst, int array_dst_count,
                                      const int layer_type, const struct ReplaceLayerInfo *replace_info)
{
	//... copy between arrays aligned arrays ...
	int dst_lay_start = replace_info->dst_lay_start;
	int dst_lay_end = replace_info->dst_lay_end;
	int src_lay_start = replace_info->src_lay_start;
	int src_n, dst_n;


	if ((array_dst && array_src) && (array_src_count == array_dst_count)) {
		int i;

		void *ptr;

		for (i = 0; i < array_dst_count; i++) {
			BMElem *ele_src = array_src[i];
			BMElem *ele_dst = array_dst[i];

			for (dst_n = dst_lay_start, src_n = src_lay_start; dst_n <= dst_lay_end; dst_n++, src_n++) {
				ptr = CustomData_bmesh_get_n(cd_src, ele_src->head.data, layer_type, src_n);
				CustomData_bmesh_set_n(cd_dst, ele_dst->head.data, layer_type, dst_n, ptr);
			}

		}
	}

	else {
		printf("%s: %d != %d\n", __func__, array_src_count, array_dst_count);
	}
}

/**
 * @brief BM_mesh_transfer_mapped
 * Transfer CD layer of type @param layer_type between @param htype elements from the @param bm_src to @param bm_dst
 * Similar number of elements in the source and destination is a must
 * The transfer takes only the indices into consideration (unwanted results would happen if order wasn't maintained)
 *
 * @param bm_src			source bmesh
 * @param bm_dst			destination bmesh
 * @param htype				element type
 * @param layer_type		CD layer type
 * @param replace_info		determines the layer-group mapping for the transfer
 */

static void BM_mesh_transfer_aligned(BMesh *bm_src, BMesh *bm_dst, const char htype, const int layer_type,
                                     const struct ReplaceLayerInfo *replace_info)
{
	int array_dst_len;
	int array_src_len;

	BMElem **array_src;
	BMElem **array_dst;

	CustomData *cd_dst;
	CustomData *cd_src;

	switch (htype) {
		case BM_VERT:
			array_src = BM_iter_as_arrayN(bm_src, BM_VERTS_OF_MESH, NULL, &array_src_len, NULL, 0);
			array_dst = BM_iter_as_arrayN(bm_dst, BM_VERTS_OF_MESH, NULL, &array_dst_len, NULL, 0);
			break;

		case BM_LOOP:
			array_src = MEM_mallocN(sizeof(*array_src) * bm_src->totloop, "array_src bmesh_data_transfer.c");
            array_dst = MEM_mallocN(sizeof(*array_dst) * bm_dst->totloop, "array_dst bmesh_data_transfer.c");

			array_src_len = BM_iter_loops_as_array(bm_src, array_src, bm_src->totloop);
			array_dst_len = BM_iter_loops_as_array(bm_dst, array_dst, bm_dst->totloop);

			break;
	}


	/* this could be its own function even */
	switch (htype) {
		case BM_VERT:
			cd_src = &bm_src->vdata;
			cd_dst = &bm_dst->vdata;
			break;

#if 0
		case BM_EDGE:
			cd_src = &bm_src->edata;
			cd_dst = &bm_dst->edata;
			break;

		case BM_FACE:
			cd_src = &bm_src->pdata;
			cd_dst = &bm_dst->pdata;
			break;
#endif
		case BM_LOOP:
			cd_src = &bm_src->ldata;
			cd_dst = &bm_dst->ldata;
			break;
	}

	BM_mesh_cd_transfer_array(cd_src, array_src, array_src_len, cd_dst, array_dst, array_dst_len, layer_type, replace_info);
}

//--------------topology transfer definitions---------

/**
 * @brief BM_mesh_cd_transfer_mapped
 * Given the mapping -@param index_mapping- between source elements -@param array_src-
 *		and dest elements -@param array_dst-, copy the CD of type @param layer_type from @param cd_src to @param cd_dst
 * Layers to be copied must be given as the @param replace_info
 *
 * @param cd_src			source cd layer
 * @param array_src			array of source elements
 * @param array_src_count	number of provided source elements
 * @param cd_dst			destination cd layer
 * @param array_dst			array of destination elements
 * @param array_dst_count	number of provided destination elements
 * @param layer_type		CD layer type
 * @param replace_info		layers to replace
 * @param index_mapping		mapping between source and destination elements
 *
 * Keep in sync with BM_mesh_cd_transfer_array
 */

static void BM_mesh_cd_transfer_mapped(CustomData *cd_src, BMElem **array_src, int array_src_count,
                                CustomData *cd_dst, BMElem **array_dst, int array_dst_count,
                                const int layer_type, const struct ReplaceLayerInfo *replace_info,
                                int *index_mapping)
{
	//... copy between arrays with a mapping! ...
	int dst_lay_start = replace_info->dst_lay_start;
	int dst_lay_end = replace_info->dst_lay_end;
	int src_lay_start = replace_info->src_lay_start;
	int src_n, dst_n;

	if ((array_dst && array_src) && (array_src_count == array_dst_count)) {
		int i;

		void *ptr;

		for (i = 0; i < array_dst_count; i++) {

			BMElem *ele_src;
			BMElem *ele_dst;

			if (index_mapping[i] == -1) {	//shall never be reached!!
				continue;
			}

			ele_src = array_src[index_mapping[i]];
			ele_dst = array_dst[i];

			for (dst_n = dst_lay_start, src_n = src_lay_start; dst_n <= dst_lay_end; dst_n++, src_n++) {
				ptr = CustomData_bmesh_get_n(cd_src, ele_src->head.data, layer_type, src_n);
				CustomData_bmesh_set_n(cd_dst, ele_dst->head.data, layer_type, dst_n, ptr);
			}
		}
	}

	else {
		printf("%s: %d != %d\n", __func__, array_src_count, array_dst_count);
	}
}

/**
 * @brief BM_mesh_transfer_mapped
 * Transfer CD layer of type @param layer_type between @param htype elements from the @param bm_src to @param bm_dst
 * Similar number of elements in the source and destination is a must
 * It's OK if the elements weren't in order
 *
 * @param bm_src			source bmesh
 * @param bm_dst			destination bmesh
 * @param htype				element type
 * @param layer_type		CD layer type
 * @param replace_info		determines the layer-group mapping for the transfer
 *
 * Keep in sync with BM_mesh_transfer_aligned
 */

static void BM_mesh_transfer_mapped(BMesh *bm_src, BMesh *bm_dst, const char htype, const int layer_type,
                                     const struct ReplaceLayerInfo *replace_info)
{
	int array_dst_len;
	int array_src_len;

	BMElem **array_src;
	BMElem **array_dst;

	CustomData *cd_dst;
	CustomData *cd_src;

	BMFace *f;
	BMLoop *l;
	BMIter fiter, liter;

	int *index_mapping;
	switch (htype) {
		case BM_VERT:
			array_src = BM_iter_as_arrayN(bm_src, BM_VERTS_OF_MESH, NULL, &array_src_len, NULL, 0);
			array_dst = BM_iter_as_arrayN(bm_dst, BM_VERTS_OF_MESH, NULL, &array_dst_len, NULL, 0);
			index_mapping = BM_mesh_mapping(bm_src, bm_dst, BM_VERT);
			break;

		case BM_LOOP:
			set_loop_indices(bm_dst);
			set_loop_indices(bm_src);

			array_src_len = bm_src->totloop;
			array_dst_len = bm_dst->totloop;
			array_src = MEM_mallocN(sizeof(*array_src) * array_src_len, "array_src bmesh_data_transfer.c");
            array_dst = MEM_mallocN(sizeof(*array_dst) * array_dst_len, "array_dst bmesh_data_transfer.c");

			BM_ITER_MESH (f, &fiter, bm_src, BM_FACES_OF_MESH) {
				BM_ITER_ELEM (l, &liter, f, BM_LOOPS_OF_FACE) {
					array_src[BM_elem_index_get(l)] = (BMElem*) l;
				}
			}

			BM_ITER_MESH (f, &fiter, bm_dst, BM_FACES_OF_MESH) {
				BM_ITER_ELEM (l, &liter, f, BM_LOOPS_OF_FACE) {
					array_dst[BM_elem_index_get(l)] = (BMElem*) l;
				}
			}
			index_mapping = BM_mesh_mapping(bm_src, bm_dst, BM_VERT);
			index_mapping = BM_transform_index_mapping(bm_src, array_dst, array_dst_len, index_mapping, BM_VERT, BM_LOOP);

			break;

		default:
			BLI_assert(0);
			break;
	}


	/* this could be its own function even */
	switch (htype) {
		case BM_VERT:
			cd_src = &bm_src->vdata;
			cd_dst = &bm_dst->vdata;
			break;
#if 0
		case BM_EDGE:
			cd_src = &bm_src->edata;
			cd_dst = &bm_dst->edata;
			break;
		case BM_FACE:
			cd_src = &bm_src->pdata;
			cd_dst = &bm_dst->pdata;
			break;
#endif

		case BM_LOOP:
			cd_src = &bm_src->ldata;
			cd_dst = &bm_dst->ldata;
			break;

		default:
			BLI_assert(0);
			break;
	}

	BM_mesh_cd_transfer_mapped(cd_src, array_src, array_src_len, cd_dst, array_dst, array_dst_len, layer_type, replace_info,
	                           index_mapping);

	MEM_freeN(array_dst);
	MEM_freeN(array_src);
	MEM_freeN(index_mapping);
}

//--------------interpolation transfer definitions---------

/**
 * @brief BM_mesh_cd_transfer_mapped
 * Given the mapping -@param index_mapping- between source elements -@param array_src-
 *		and dest elements -@param array_dst-, copy the CD of type @param layer_type from @param cd_src to @param cd_dst
 * Layers to be copied must be given as the @param replace_info
 *
 * @param cd_src			source cd layer
 * @param array_src			array of source elements
 * @param array_src_count	number of provided source elements
 * @param cd_dst			destination cd layer
 * @param array_dst			array of destination elements
 * @param array_dst_count	number of provided destination elements
 * @param layer_type		CD layer type
 * @param replace_info		layers to replace
 * @param index_mapping		mapping between source and destination elements
 *
 * Keep in sync with BM_mesh_cd_transfer_array
 */

static void BM_mesh_cd_transfer_interpolated(CustomData *cd_src, BMElem **array_src, int array_src_count,
                                CustomData *cd_dst, BMElem **array_dst, int array_dst_count,
                                const int layer_type, const struct ReplaceLayerInfo *replace_info,
                                int **index_mapping, int *index_mapping_count, float **index_mapping_weights)
{
	//... copy between arrays with a mapping! ...
	int dst_lay_start = replace_info->dst_lay_start;
	int dst_lay_end = replace_info->dst_lay_end;
	int src_lay_start = replace_info->src_lay_start;
	int src_n, dst_n;
	int *ele_src_list;

	if (array_dst && array_src) {
		int i, k;

		// array_src_count is the maximum source count that could be achieved
		void **ptr = MEM_mallocN(sizeof(char*) * array_src_count, "ptr bmesh_data_transfer.c");

		for (i = 0; i < array_dst_count; i++) {

			BMElem *ele_src;
			BMElem *ele_dst;

			if (index_mapping[i] == NULL) {	//shall never be reached!!
				continue;
			}

			ele_src_list = index_mapping[i];
			ele_dst = array_dst[i];

			for (dst_n = dst_lay_start, src_n = src_lay_start; dst_n <= dst_lay_end; dst_n++, src_n++) {
				//for every source_ele fetch the pointers list
				for (k = 0; k < index_mapping_count[i]; k++) {
					ele_src = array_src[ele_src_list[k]];
					ptr[k] = CustomData_bmesh_get_n(cd_src, ele_src->head.data, layer_type, src_n);
				}

				CustomData_bmesh_interp_n(cd_dst, ptr, index_mapping_weights[i], NULL, index_mapping_count[i],
				                          ele_dst->head.data, dst_n);
			}
		}

		MEM_freeN(ptr);
	}

	else {
		printf("%s: either array_dst or array_src point to null!\n", __func__);
	}
}

static void BM_mesh_transfer_interpolated(BMesh *bm_src, BMesh *bm_dst, const char htype, const int layer_type,
                                     const struct ReplaceLayerInfo *replace_info)
{
	int array_dst_len;
	int array_src_len;

	BMElem **array_src;
	BMElem **array_dst;

	CustomData *cd_dst;
	CustomData *cd_src;

/*	BMFace *f;
	BMLoop *l;
	BMIter fiter, liter;
*/
	int *init_index_mapping;
	int **fin_index_mapping;
	int *fin_index_mapping_len;
	float **fin_index_mapping_weights;

	switch (htype) {

		case BM_VERT:
			array_src = BM_iter_as_arrayN(bm_src, BM_VERTS_OF_MESH, NULL, &array_src_len, NULL, 0);
			array_dst = BM_iter_as_arrayN(bm_dst, BM_VERTS_OF_MESH, NULL, &array_dst_len, NULL, 0);

			init_index_mapping = BM_mesh_mapping(bm_src, bm_dst, BM_FACE);
			BM_transform_index_multi_mapping(bm_src, array_dst, array_dst_len, init_index_mapping, BM_FACE, BM_VERT,
			                                 &fin_index_mapping, &fin_index_mapping_len, &fin_index_mapping_weights);

			break;

		case BM_LOOP:
			set_loop_indices(bm_dst);
			set_loop_indices(bm_src);

			array_src = MEM_mallocN(sizeof(*array_src) * bm_src->totloop, "array_src bmesh_data_transfer.c");
            array_dst = MEM_mallocN(sizeof(*array_dst) * bm_dst->totloop, "array_dst bmesh_data_transfer.c");

			array_src_len = BM_iter_loops_as_array(bm_src, array_src, bm_src->totloop);
			array_dst_len = BM_iter_loops_as_array(bm_dst, array_dst, bm_dst->totloop);

			init_index_mapping = BM_mesh_mapping(bm_src, bm_dst, BM_FACE);
			BM_transform_index_multi_mapping(bm_src, array_dst, array_dst_len, init_index_mapping, BM_FACE, BM_LOOP,
			                                 &fin_index_mapping, &fin_index_mapping_len, &fin_index_mapping_weights);

			break;

		default:
			BLI_assert(0);
			break;
	}


	/* this could be its own function even */
	switch (htype) {
		case BM_VERT:
			cd_src = &bm_src->vdata;
			cd_dst = &bm_dst->vdata;
			break;
#if 0
		case BM_EDGE:
			cd_src = &bm_src->edata;
			cd_dst = &bm_dst->edata;
			break;
		case BM_FACE:
			cd_src = &bm_src->pdata;
			cd_dst = &bm_dst->pdata;
			break;
#endif

		case BM_LOOP:
			cd_src = &bm_src->ldata;
			cd_dst = &bm_dst->ldata;
			break;

		default:
			BLI_assert(0);
			break;
	}

	BM_mesh_cd_transfer_interpolated(cd_src, array_src, array_src_len, cd_dst, array_dst, array_dst_len, layer_type,
	                                 replace_info, fin_index_mapping, fin_index_mapping_len, fin_index_mapping_weights);

	MEM_freeN(array_dst);
	MEM_freeN(array_src);
	MEM_freeN(init_index_mapping);
	MEM_freeN(fin_index_mapping);
	///we may need to free each element by itself
	MEM_freeN(fin_index_mapping_len);
	MEM_freeN(fin_index_mapping_weights);
}

//---------------helping functions definitions-----------

/**
 * @brief BM_mesh_mapping
 * Given a certain element type return a mapping pointer for each element in the destination to an element in the source
 * The pointer would have the same number of elements as the destination
 * This is considered 1(destination):N(source) mapping of the elements
 *
 * @param bm_src	source bmesh
 * @param bm_dst	destination bmesh
 * @param htype		element type to map
 * @return			mapping pointer
 *
 * Currently supported types are BMVert and BMLoop
 */

static void *BM_mesh_mapping(BMesh *bm_src, BMesh *bm_dst, const char htype)
{
	BMVert *v_src, *v_dst;
	BMIter iter;

	BMFace *f_src, *f_dst;
	BMIter fiter;
	float co[3];

#if 0
	BMLoop *l_dst, *l_src;
	BMIter fiter, liter;
	float f_mid_dst[3];
#endif

	int a;
	int *index_mapping;

	//does the bm_src get affected when we do_tesselation ?
	BMBVHTree *bmtree_src;

	BMEditMesh *em_src;// = BKE_editmesh_create(bm_src, true);
	em_src = BKE_editmesh_create(bm_src, true);
	bmtree_src = BKE_bmbvh_new_from_editmesh(em_src, 0, NULL, false);

	switch (htype) {
		case BM_VERT:
			index_mapping = MEM_mallocN(bm_dst->totvert * sizeof(*index_mapping), "index_mapping bmesh_data_transfer.c");
			BM_ITER_MESH_INDEX (v_dst, &iter, bm_dst, BM_VERTS_OF_MESH, a) {

				v_src = BKE_bmbvh_find_vert_closest(bmtree_src, v_dst->co, FLT_MAX);
				if (v_src != NULL) {
					index_mapping[a] = BM_elem_index_get(v_src);
				}
				else {
					index_mapping[a] = -1;
				}

			}

			break;
#if 0
		case BM_LOOP:
			index_mapping = MEM_mallocN(bm_dst->totloop * sizeof(*index_mapping), "index_mapping bmesh_data_transfer.c");

			BM_ITER_MESH_INDEX (f_dst, &fiter, bm_dst, BM_FACES_OF_MESH, a) {

				BM_face_calc_center_mean(f_dst, f_mid_dst);

				f_src = BKE_bmbvh_find_face_closest(bmtree_src, f_mid_dst, FLT_MAX);

				if (f_src != NULL) {
					BM_ITER_ELEM (l_dst, &liter, f_dst, BM_LOOPS_OF_FACE) {
						l_src = BM_face_find_best_tan_match_loop(f_src, l_dst);
						if (l_src != NULL) {
							index_mapping[BM_elem_index_get(l_dst)] = BM_elem_index_get(l_src);
						}
					}
				}
				else {
					BM_ITER_ELEM (l_dst, &liter, f_dst, BM_LOOPS_OF_FACE) {
						index_mapping[BM_elem_index_get(l_dst)] = -1;
					}
				}
			}

			break;

		case BM_EDGE:
			break;
#endif
		case BM_FACE:
			index_mapping = MEM_mallocN(bm_dst->totface * sizeof(*index_mapping), "index_mapping bmesh_data_transfer.c");
			BM_ITER_MESH_INDEX (f_dst, &fiter, bm_dst, BM_FACES_OF_MESH, a) {

				BM_face_calc_center_mean(f_dst, co);

				f_src = BKE_bmbvh_find_face_closest(bmtree_src, co, FLT_MAX);
				if (f_src != NULL) {
					index_mapping[a] = BM_elem_index_get(f_src);
				}
				else {
					index_mapping[a] = -1;
				}

			}

			break;

		default:
			BKE_bmbvh_free(bmtree_src);
			MEM_freeN(em_src);
			BLI_assert(0);
			return false;
			//break;
	}


	BKE_bmbvh_free(bmtree_src);
	MEM_freeN(em_src);

	return index_mapping;
}

static void set_loop_indices(BMesh *bm)
{
	BMLoop *l;
	BMFace *f;
	BMIter fiter, liter;

	int b;

	b = 0;
	BM_ITER_MESH (f, &fiter, bm, BM_FACES_OF_MESH) {
		BM_ITER_ELEM (l, &liter, f, BM_LOOPS_OF_FACE) {
			BM_elem_index_set(l, b);
			b++;
		}
	}
}

/**
 * @brief BM_transform_index_mapping
 * Uses a prev-mapping @param index_mapping_in between certain elements type @param htype_from to get a new-mapping of
 * the type @param htype_to,
 * Requires the mapping elements @param array_dst to be given, in the same order as the index_mapping_in,
 * Used mainly to ransform from face/vert/edge mapping to loop mapping
 *
 * @param bm_src				source bmesh we shall fetch the loops from
 * @param array_dst				given array of destination bmesh elements
 * @param array_dst_count		cound of the given array
 * @param index_mapping_in		prev-mapping between @param array_dst and the @param bm_src's elements
 * @param htype_from			type of the @param array_dst
 * @param htype_to				type of mapping we seek
 * @return index_mapping_out	new-mapping between the destination's and the source's @param htype_to elements
 *
 * Currently supported conversions are
 *	BM_VERT to BM_LOOP
 * TODO
 *	support BM_EDGE and BM_FACE to BM_LOOP
 */

static int *BM_transform_index_mapping(BMesh *bm_src, BMElem **array_dst, int array_dst_count, int *index_mapping_in,
                                       int htype_from, int htype_to)
{
	int i;
	BMElem *ele_dst;
	BMElem *ele_src;
	BMLoop *l_dst;
	BMVert *v_src;

	int v_src_count = bm_src->totvert;
	BMVert **v_array_src = MEM_mallocN(v_src_count * sizeof(*v_array_src),
	                                   "v_array_src bmesh_data_transfer.c");	//lookup array for the vertices to avoid
																				//using the BM_vert_at_index per element

	int f_src_count = bm_src->totface;
	BMFace **f_array_src = MEM_mallocN(f_src_count * sizeof(*f_array_src),
	                                   "f_array_src bmesh_data_transfer.c");

	int *index_mapping_out = MEM_mallocN(array_dst_count * sizeof(*index_mapping_out),
	                                     "index_mapping_out bmesh_data_transfer.c");

	BM_iter_as_array(bm_src, BM_VERTS_OF_MESH, NULL, (void **)v_array_src, bm_src->totvert);
	BM_iter_as_array(bm_src, BM_FACES_OF_MESH, NULL, (void **)f_array_src, bm_src->totface);


	if (htype_from == BM_VERT && htype_to == BM_LOOP) {
		for (i = 0; i < array_dst_count; i++) {	//for each destination loop
			int v_src_index;

			//get the respective destination vertex index
			ele_dst = array_dst[i];
			l_dst = (BMLoop*) ele_dst;
			v_src_index = index_mapping_in[BM_elem_index_get(l_dst->v)];

			//check it has got a mapping
			if (v_src_index == -1) { //should never be reached
				index_mapping_out[i] = -1;
				continue;
			}

			//find the best loop match within the respective source vertix
			//is it safe to extract the bmloops from bmelems
//			v_src = BM_vert_at_index(bm_src, v_src_index);
			v_src = v_array_src[v_src_index];
			ele_src = (BMElem*) BM_vert_find_best_tan_match_loop(v_src, (BMLoop*) ele_dst);

			if (ele_src == NULL) {	//should never be reached
				index_mapping_out[i] = -1;
				continue;
			}

			index_mapping_out[i] = BM_elem_index_get(ele_src);
		}
	}

	else {
		BLI_assert(0);
		return NULL;
	}

	return index_mapping_out;
}

/**
 * uses preset loop indices
 * this function results in 1:N mapping
*/

static bool BM_transform_index_multi_mapping(BMesh *bm_src, BMElem **array_dst, int array_dst_count, int *index_mapping_in,
                                      int htype_from, int htype_to, int ***r_index_mapping_out,
                                      int **r_index_mapping_out_len, float ***r_index_mapping_out_weights)
{
	BMElem *ele_dst;
	BMFace **f_array_src = MEM_mallocN(bm_src->totface * sizeof(*f_array_src),
	                                   "f_array_src bmesh_data_transfer.c");
	int i;

	int **index_mapping_out = MEM_mallocN(array_dst_count * sizeof(*index_mapping_out),
	                                      "r_index_mapping_out bmesh_data_transfer.c");
	int *index_mapping_out_len = MEM_mallocN(array_dst_count * sizeof(*index_mapping_out_len),
	                                         "r_index_mapping_out_len bmesh_data_transfer.c");
	float **index_mapping_out_weights = MEM_mallocN(array_dst_count * sizeof(*index_mapping_out_weights),
	                                                "r_index_mapping_out_weights bmesh_data_transfer.c");

	BM_iter_as_array(bm_src, BM_FACES_OF_MESH, NULL, (void **)f_array_src, bm_src->totface);

	//transformation rules: every dst loop should get all the f_src's loops with their weights
	if (htype_from == BM_FACE && htype_to == BM_LOOP) {
		int f_src_index, f_dst_index;
		BMLoop *l_dst;
		BMFace *f_src;
		float (*co_list)[3];

		for (i = 0; i < array_dst_count; i++) {	//for each destination loop

			//initial setting of zero
			index_mapping_out_len[i] = 0;

			//get a loop from the destination array
			ele_dst = array_dst[i];
			l_dst = (BMLoop*) ele_dst;

			//get the destination face index
			f_dst_index = BM_elem_index_get(l_dst->f);

			//get the source face index
			f_src_index = index_mapping_in[f_dst_index];

			//check it has got a mapping
			if (f_src_index == -1) { //should never be reached in the current scenario
				///fix this!
				index_mapping_out[i] = NULL;
				continue;
			}

			//get the source face
			f_src = f_array_src[f_src_index];

			//get the respective loops
			BM_get_face_loops_as_indices(f_src, &index_mapping_out[i], NULL);

			//we shouldn't take the weight calc out of this fn not to lose the info about which faces are the loops from
			//get co list for the source loops
			BM_get_face_coord_list(&co_list, f_src);

			index_mapping_out_weights[i] = MEM_mallocN(sizeof(*index_mapping_out_weights[i]) * f_src->len,
			                                           "r_index_mapping_out_weights[i] bmesh_data_transfer.c");

			interp_weights_poly_v3(index_mapping_out_weights[i], co_list, f_src->len, l_dst->v->co);
			index_mapping_out_len[i] += f_src->len;
		}
	}

	//transformation rules: every dst vert should get all the verts in the corresponding mapped faces to the faces that
	//share this vert
	else if (htype_from == BM_FACE && htype_to == BM_VERT) {	//the mapping Should result in N:M then 1:N mapping
		int f_src_index, f_dst_index;
		BMVert *v_dst;
		BMFace *f_dst, *f_src;
		BMIter iter;
		int *mapping_list;
		float (*co_list)[3];
		float (*total_co_list)[3] = MEM_mallocN(sizeof(*total_co_list), "total_co_list bmesh_data_transfer.c");

		//sanity allocation: to avoid a segmentation fault upon freeing unallocated memory
		co_list = MEM_mallocN(sizeof(*co_list), "co_list bmesh_data_transfer.c");
		mapping_list = MEM_mallocN(sizeof(*mapping_list), "mapping_list bmesh_data_transfer.c");

		for (i = 0; i < array_dst_count; i++) {	//for each destination vert

			//initial allocation with 1
			index_mapping_out[i] = MEM_mallocN(sizeof(**index_mapping_out),
			                                   "index_mapping_out[i] bmesh_data_transfer.c");

			//initial setting of zero
			index_mapping_out_len[i] = 0;

			//get a vert from the destination array
			ele_dst = array_dst[i];
			v_dst = (BMVert*) ele_dst;

			//todo: remove this part to preceed the vertex list iteration, to avoid searching the same face twice
			//get the destination faces
			BM_ITER_ELEM (f_dst, &iter, v_dst, BM_FACES_OF_VERT) {

				//get the destination face index
				f_dst_index = BM_elem_index_get(f_dst);

				//get the source face index
				f_src_index = index_mapping_in[f_dst_index];

				//check it has got a mapping
				if (f_src_index == -1) { //should never be reached in the current scenario
					continue;
				}

				//get the source face
				f_src = f_array_src[f_src_index];

				//append the respective verts
				BM_get_face_verts_as_indices(f_src, &mapping_list, NULL);

				append_index_list(&index_mapping_out[i], index_mapping_out_len[i], mapping_list, f_src->len);

				//get co list for the source verts
				BM_get_face_coord_list(&co_list, f_src);

				append_co_list(&total_co_list, index_mapping_out_len[i], co_list, f_src->len);

				index_mapping_out_len[i] += f_src->len;

			}

			//free allocated memory if we didn't receive a mapping from any of the surrounding faces
			if (index_mapping_out_len[i] == 0) {
				MEM_freeN(index_mapping_out[i]);
				continue;
			}

			index_mapping_out_weights[i] = MEM_mallocN(sizeof(**index_mapping_out_weights) * index_mapping_out_len[i],
			                                           "r_index_mapping_out_weights[i] bmesh_data_transfer.c");

			interp_weights_poly_v3(index_mapping_out_weights[i], total_co_list, index_mapping_out_len[i], v_dst->co);
		}

		MEM_freeN(co_list);
		MEM_freeN(total_co_list);
		MEM_freeN(mapping_list);
	}

	else {
		BLI_assert(0);
		return false;
	}

	*r_index_mapping_out = index_mapping_out;
	*r_index_mapping_out_len = index_mapping_out_len;
	*r_index_mapping_out_weights = index_mapping_out_weights;

	return true;
}

/**
 * @brief BM_vert_find_best_tan_match_loop
 * given a vertex and a loop find the the best vert's loop that matches the given loop's orientation
 *
 * @param v_src		given vert
 * @param l_dst		any loop
 * @return l		the best matching vert's loop to the given loop
 */

static BMLoop* BM_vert_find_best_tan_match_loop(BMVert *v_src, BMLoop *l_dst) {

	BMLoop *l, *l_src;
	BMIter liter;
	float l_dst_tan[3], l_src_tan[3];

	float dot_product, prev_dot_product;

	if (BM_vert_edge_count(v_src) == 0) {
		return NULL;
	}

	dot_product = -1;
	prev_dot_product = -2;

	BM_loop_calc_face_tangent(l_dst, l_dst_tan);

	BM_ITER_ELEM (l_src, &liter, v_src, BM_LOOPS_OF_VERT) {
		BM_loop_calc_face_tangent(l_src, l_src_tan);

		dot_product = dot_v3v3(l_src_tan, l_dst_tan);

		if (dot_product > prev_dot_product) {
			l = l_src;
			prev_dot_product = dot_product;
		}

	}

	return l;
}

/**
 * @brief BM_iter_loops_as_array given
 * fill the array with bmesh's loops, and return the number of those loops
 *
 * @param bm		bmesh we would use for the array
 * @param array		array to be filled
 * @param len		number of bmesh loops, currently used for sanity checking (may not be needed)
 * @return i		number of loops filled into the array
 */

static int BM_iter_loops_as_array(BMesh *bm, BMElem **array, int len)
{
	BMLoop *l;
	BMFace *f;
	BMIter fiter, liter;
	int i = 0;

	/* sanity check */
	if (len > 0) {
		BM_ITER_MESH (f, &fiter, bm, BM_FACES_OF_MESH) {
			BM_ITER_ELEM (l, &liter, f, BM_LOOPS_OF_FACE) {
				array[i] = (BMElem*) l;
				i++;
				if (i == len) {
					return len;
				}
			}
		}
	}

	return i;
}

static void BM_get_face_coord_list(float (**r_co_list)[3], BMFace *f)
{
	BMVert *v;
	BMIter fiter;
	int i;
	float (*co_list)[3];

	co_list = MEM_mallocN((sizeof(*co_list)) * f->len, "co_list bmesh_data_transfer.c");

	BM_ITER_ELEM_INDEX(v, &fiter, f, BM_VERTS_OF_FACE, i) {
		copy_v3_v3(co_list[i], v->co);
	}

	*r_co_list = co_list;
}

/**
 * uses preset loop indices
 */

static void BM_get_face_loops_as_indices(BMFace *f, int **r_loop_array, int *r_loop_array_len)
{
	BMIter liter;
	BMLoop *l;
	int *loop_array = MEM_mallocN(sizeof(*loop_array) * f->len, "loop_array bmesh_data_transfer.c");
	int i;

	BM_ITER_ELEM_INDEX (l, &liter, f, BM_LOOPS_OF_FACE, i) {
		loop_array[i] = BM_elem_index_get(l);
	}

	if (r_loop_array_len) {
		*r_loop_array_len = i;
	}

	*r_loop_array = loop_array;
}

static void BM_get_face_verts_as_indices(BMFace *f, int **r_vert_array, int *r_vert_array_len)
{
	BMIter iter;
	BMVert *v;
	int *vert_array = MEM_mallocN(sizeof(*vert_array) * f->len, "vert_array bmesh_data_transfer.c");
	int i;

	BM_ITER_ELEM_INDEX (v, &iter, f, BM_VERTS_OF_FACE, i) {
		vert_array[i] = BM_elem_index_get(v);
	}

	if (r_vert_array_len) {
		*r_vert_array_len = i;
	}

	*r_vert_array = vert_array;
}

/**
 * r_out_list must be pre-allocated with MEM_mallocN to any size
 */

static void append_index_list(int **r_out_list, const int out_list_len, int *in_list, const int in_list_len)
{
	int i;
	int *out_list = *r_out_list;
	out_list = MEM_reallocN(out_list, sizeof(*out_list) * (out_list_len + in_list_len));

	for (i = 0; i < in_list_len; i++) {
		out_list[out_list_len + i] = in_list[i];
	}

	*r_out_list = out_list;
}

/**
 * r_out_list must be pre-allocated with MEM_mallocN to any size
 */

static void append_co_list(float (**r_out_list)[3], const int out_list_len, float (*in_list)[3], const int in_list_len)
{
	int i;
	float (*out_list)[3] = *r_out_list;
	out_list = MEM_reallocN(out_list, sizeof(*out_list) * (out_list_len + in_list_len));

	for (i = 0; i < in_list_len; i++) {
		copy_v3_v3(out_list[out_list_len + i], in_list[i]);
	}

	*r_out_list = out_list;
}

#if 0

static void BM_get_face_elems_as_indices(BMFace *f, int **r_elem_array, int *r_elem_array_len, int htype)
{
	BMIter iter;
	BMElem *ele;
	int *elem_array = MEM_mallocN((sizeof(*elem_array) * f->len), "elem_array bmesh_data_transfer.c");
	int i;

	switch (htype) {
		case BM_VERT:
			BM_ITER_ELEM_INDEX (ele, &iter, f, BM_LOOPS_OF_FACE, i) {
				elem_array[i] = BM_elem_index_get(ele);
			}
			break;

		case BM_LOOP:
			BM_ITER_ELEM_INDEX (ele, &iter, f, BM_LOOPS_OF_FACE, i) {
				elem_array[i] = BM_elem_index_get(ele);
			}
			break;
		case BM_EDGE:
			BM_ITER_ELEM_INDEX (ele, &iter, f, BM_EDGES_OF_FACE, i) {
				elem_array[i] = BM_elem_index_get(ele);
			}
			break;
	}

	if (r_elem_array_len) {
		*r_elem_array_len = i;
	}

	*r_elem_array = elem_array;
}

void BM_get_face_loops(BMFace *f, BMLoop **r_loop_array, int *r_loop_array_len)
{
	BMIter liter;
	BMLoop *l;
	r_loop_array = MEM_mallocN((sizeof(*r_loop_array) * f->len), "loop_array bmesh_data_transfer.c");
	int i;

	BM_ITER_ELEM_INDEX (l, &liter, bm, BM_LOOPS_OF_FACE, i) {
		r_loop_array[i] = l;
	}

	r_loop_array_len = i;
}

/**
 * @brief BM_face_find_best_tan_match_loop
 * Given a face @param f_src get the best matching face's loop -@param l- to another loop -@param l_dst-
 * @param f_src		given face
 * @param l_dst		any loop
 * @return l		the best matching face's loop to the given loop
 */

static BMLoop* BM_face_find_best_tan_match_loop(BMFace *f_src, BMLoop *l_dst) {

	BMLoop *l, *l_src;
	BMIter liter;
	float l_dst_tan[3], l_src_tan[3];

	float dot_product, prev_dot_product;

	if (f_src->len == 0)
		return NULL;

	dot_product = -1;
	prev_dot_product = -2;

	BM_loop_calc_face_tangent(l_dst, l_dst_tan);

	BM_ITER_ELEM (l_src, &liter, f_src, BM_LOOPS_OF_FACE) {
		BM_loop_calc_face_tangent(l_src, l_src_tan);

		dot_product = dot_v3v3(l_src_tan, l_dst_tan);

		if (dot_product > prev_dot_product) {
			l = l_src;
			prev_dot_product = dot_product;
		}

	}

	return l;
}
#endif
