

#include "OpenColorIO_AE.h"




static PF_Err 
About (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_SPRINTF(	out_data->return_msg, 
				"%s - %s\r\rwritten by %s\r\rv%d.%d - %s\r\r%s\r%s",
				NAME,
				DESCRIPTION,
				AUTHOR, 
				MAJOR_VERSION, 
				MINOR_VERSION,
				RELEASE_DATE,
				COPYRIGHT,
				WEBSITE);
				
	return PF_Err_NONE;
}

static PF_Err 
GlobalSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	//	We do very little here.
		
	out_data->my_version 	= 	PF_VERSION(	MAJOR_VERSION, 
											MINOR_VERSION,
											BUG_VERSION, 
											STAGE_VERSION, 
											BUILD_VERSION);

	out_data->out_flags 	= 	PF_OutFlag_DEEP_COLOR_AWARE		|
								PF_OutFlag_PIX_INDEPENDENT		|
								PF_OutFlag_CUSTOM_UI			|
								PF_OutFlag_USE_OUTPUT_EXTENT;

	out_data->out_flags2 	=	PF_OutFlag2_PARAM_GROUP_START_COLLAPSED_FLAG |
								PF_OutFlag2_SUPPORTS_SMART_RENDER	|
								PF_OutFlag2_FLOAT_COLOR_AWARE;
	
	return PF_Err_NONE;
}

static PF_Err
ParamsSetup(
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output)
{
	PF_Err 			err = PF_Err_NONE;
	PF_ParamDef		def;


	// readout
	AEFX_CLR_STRUCT(def);
	// we can time_vary once we're willing to print and scan ArbData text
	def.flags = PF_ParamFlag_CANNOT_TIME_VARY;
	
	ArbNewDefault(in_data, out_data, NULL, &def.u.arb_d.dephault);
	
	PF_ADD_ARBITRARY("Channel Info (Click for Dialog)",
						UI_CONTROL_WIDTH,
						UI_CONTROL_HEIGHT,
						PF_PUI_CONTROL,
						def.u.arb_d.dephault,
						OCIO_DATA,
						NULL);

	out_data->num_params = OCIO_NUM_PARAMS;

	// register custom UI
	if (!err) 
	{
		PF_CustomUIInfo			ci;

		AEFX_CLR_STRUCT(ci);
		
		ci.events				= PF_CustomEFlag_EFFECT;
 		
		ci.comp_ui_width		= ci.comp_ui_height = 0;
		ci.comp_ui_alignment	= PF_UIAlignment_NONE;
		
		ci.layer_ui_width		= 0;
		ci.layer_ui_height		= 0;
		ci.layer_ui_alignment	= PF_UIAlignment_NONE;
		
		ci.preview_ui_width		= 0;
		ci.preview_ui_height	= 0;
		ci.layer_ui_alignment	= PF_UIAlignment_NONE;

		err = (*(in_data->inter.register_ui))(in_data->effect_ref, &ci);
	}


	return err;
}

static PF_Err
SequenceSetup (
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err err = PF_Err_NONE;
	
	SequenceData *seq_data = NULL;
	
	// set up sequence data
	if( (in_data->sequence_data == NULL) )
	{
		out_data->sequence_data = PF_NEW_HANDLE( sizeof(SequenceData) );
		
		seq_data = (SequenceData *)PF_LOCK_HANDLE(out_data->sequence_data);
	}
	else // reset pre-existing sequence data
	{
		if( PF_GET_HANDLE_SIZE(in_data->sequence_data) != sizeof(SequenceData) )
		{
			PF_RESIZE_HANDLE(sizeof(SequenceData), &in_data->sequence_data);
		}
			
		seq_data = (SequenceData *)PF_LOCK_HANDLE(in_data->sequence_data);
	}
	
	
	seq_data->ready = FALSE;
	
	
	PF_UNLOCK_HANDLE(in_data->sequence_data);
	
	return err;
}


static PF_Err 
SequenceSetdown (
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err err = PF_Err_NONE;
	
	if(in_data->sequence_data)
	{
		PF_DISPOSE_HANDLE(in_data->sequence_data);
	}

	return err;
}


static PF_Err 
SequenceFlatten (
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err err = PF_Err_NONE;

	if(in_data->sequence_data)
	{
		SequenceData *seq_data = (SequenceData *)PF_LOCK_HANDLE(in_data->sequence_data);
		
		seq_data->ready = FALSE;

		PF_UNLOCK_HANDLE(in_data->sequence_data);
	}

	return err;
}


void
SetupProcessor(ArbitraryData *arb_data, SequenceData *seq_data)
{
	if(arb_data->path[0] != '\0')
	{
		
		seq_data->ready = TRUE;
	}
}


static
PF_Boolean IsEmptyRect(const PF_LRect *r){
	return (r->left >= r->right) || (r->top >= r->bottom);
}

#ifndef mmin
	#define mmin(a,b) ((a) < (b) ? (a) : (b))
	#define mmax(a,b) ((a) > (b) ? (a) : (b))
#endif


static
void UnionLRect(const PF_LRect *src, PF_LRect *dst)
{
	if (IsEmptyRect(dst)) {
		*dst = *src;
	} else if (!IsEmptyRect(src)) {
		dst->left 	= mmin(dst->left, src->left);
		dst->top  	= mmin(dst->top, src->top);
		dst->right 	= mmax(dst->right, src->right);
		dst->bottom = mmax(dst->bottom, src->bottom);
	}
}


static PF_Err
PreRender(
	PF_InData				*in_data,
	PF_OutData				*out_data,
	PF_PreRenderExtra		*extra)
{
	PF_Err err = PF_Err_NONE;
	PF_RenderRequest req = extra->input->output_request;
	PF_CheckoutResult in_result;
	
	req.preserve_rgb_of_zero_alpha = TRUE;	//	Hey, we care.

	ERR(extra->cb->checkout_layer(	in_data->effect_ref,
									OCIO_INPUT,
									OCIO_INPUT,
									&req,
									in_data->current_time,
									in_data->time_step,
									in_data->time_scale,
									&in_result));


	UnionLRect(&in_result.result_rect, 		&extra->output->result_rect);
	UnionLRect(&in_result.max_result_rect, 	&extra->output->max_result_rect);	
	
	//	Notice something missing, namely the PF_CHECKIN_PARAM to balance
	//	the old-fashioned PF_CHECKOUT_PARAM, above? 
	
	//	For SmartFX, AE automagically checks in any params checked out 
	//	during PF_Cmd_SMART_PRE_RENDER, new or old-fashioned.
	
	return err;
}

#pragma mark-


template <typename InFormat, typename OutFormat>
OutFormat Convert(InFormat in);

template <>
float Convert<A_u_char, float>(A_u_char in)
{
	return (float)in / (float)PF_MAX_CHAN8;
}

template <>
float Convert<A_u_short, float>(A_u_short in)
{
	return (float)in / (float)PF_MAX_CHAN16;
}

static inline float
Clamp(float in)
{
	return (in > 1.f ? 1.f : in < 0.f ? 0.f : in);
}

template <>
A_u_char Convert<float, A_u_char>(float in)
{
	return ( Clamp(in) * (float)PF_MAX_CHAN8 ) + 0.5f;
}

template <>
A_u_short Convert<float, A_u_short>(float in)
{
	return ( Clamp(in) * (float)PF_MAX_CHAN16 ) + 0.5f;
}



typedef struct {
	PF_InData *in_data;
	void *in_buffer;
	size_t in_rowbytes;
	void *out_buffer;
	size_t out_rowbytes;
	int width;
} IterateData;

template <typename InFormat, typename OutFormat>
static PF_Err
CopyWorld_Iterate(void *refconPV,
					A_long thread_indexL,
					A_long i,
					A_long iterationsL)
{
	PF_Err err = PF_Err_NONE;
	
	IterateData *i_data = (IterateData *)refconPV;
	PF_InData *in_data = i_data->in_data;
	
	InFormat *in_pix = (InFormat *)((char *)i_data->in_buffer + (i * i_data->in_rowbytes)); 
	OutFormat *out_pix = (OutFormat *)((char *)i_data->out_buffer + (i * i_data->out_rowbytes));
	
	for(int x=0; x < i_data->width; x++)
	{
		*out_pix++ = Convert<InFormat, OutFormat>( *in_pix++ );
	}
	
#ifdef NDEBUG
	if(thread_indexL == 0)
		err = PF_ABORT(in_data);
#endif

	return err;
}


static PF_Err
DoRender(
		PF_InData		*in_data,
		PF_EffectWorld 	*input,
		PF_ParamDef		*OCIO_data,
		PF_OutData		*out_data,
		PF_EffectWorld	*output)
{
	PF_Err				err 	= PF_Err_NONE;

	AEGP_SuiteHandler suites(in_data->pica_basicP);

	PF_WorldSuite2 *wsP = NULL;
	
	err = in_data->pica_basicP->AcquireSuite(kPFWorldSuite, kPFWorldSuiteVersion2, (const void **)&wsP);
	

	if(!err)
	{
		ArbitraryData *arb_data = (ArbitraryData *)PF_LOCK_HANDLE(OCIO_data->u.arb_d.value);
		SequenceData *seq_data = (SequenceData *)PF_LOCK_HANDLE(in_data->sequence_data);
		
		if(!seq_data->ready)
		{
			try
			{
				SetupProcessor(arb_data, seq_data);
			}
			catch(...)
			{
				seq_data->ready = FALSE;
			}
		}

		
		if(!seq_data->ready)
		{
			PF_COPY(input, output, NULL, NULL);
		}
		else
		{
			// OpenColorIO only does float worlds
			// might have to create one
			PF_EffectWorld *float_world = NULL;
			
			PF_EffectWorld temp_world_data;
			PF_EffectWorld *temp_world = NULL;
			
			
			PF_PixelFormat format;
			wsP->PF_GetPixelFormat(output, &format);
			

			if(format == PF_PixelFormat_ARGB128)
			{
				PF_COPY(input, output, NULL, NULL);
				
				float_world = output;
			}
			else
			{
				err = wsP->PF_NewWorld(in_data->effect_ref, output->width, output->height, FALSE, PF_PixelFormat_ARGB128, &temp_world_data);
				
				float_world = temp_world = &temp_world_data;


				IterateData i_data = { in_data, input->data, input->rowbytes, float_world->data, float_world->rowbytes, float_world->width * 4 };
				
				
				if(format == PF_PixelFormat_ARGB32)
					err = suites.Iterate8Suite1()->iterate_generic(float_world->height, &i_data, CopyWorld_Iterate<A_u_char, float>);
				else if(format == PF_PixelFormat_ARGB64)
					err = suites.Iterate8Suite1()->iterate_generic(float_world->height, &i_data, CopyWorld_Iterate<A_u_short, float>);
			}
			
			
			if(!err)
			{
				// OpenColorIO processing
				try
				{					
					OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
					
					OCIO::FileTransformRcPtr transform = OCIO::FileTransform::Create();
					transform = OCIO::FileTransform::Create();
					transform->setSrc((char *)arb_data->path);
					transform->setInterpolation(OCIO::INTERP_LINEAR);
					transform->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
					
					OCIO::ConstProcessorRcPtr processor = config->getProcessor(transform);
					

					PF_PixelFloat *pix = (PF_PixelFloat *)float_world->data;
					
					float *rOut = &pix->red;
					
					OCIO::PackedImageDesc img(rOut,
											float_world->width, float_world->height,
											4, sizeof(float), sizeof(PF_PixelFloat), float_world->rowbytes);
											
					processor->apply(img);
				}
				catch(...)
				{
					err = PF_Err_INTERNAL_STRUCT_DAMAGED;
				}
			}
			
			
			// copy back to non-float world and dispose
			if(temp_world)
			{
				if(!err)
				{
					IterateData i_data = { in_data, float_world->data, float_world->rowbytes, output->data, output->rowbytes, output->width * 4 };
					
					
					if(format == PF_PixelFormat_ARGB32)
						err = suites.Iterate8Suite1()->iterate_generic(output->height, &i_data, CopyWorld_Iterate<float, A_u_char>);
					else if(format == PF_PixelFormat_ARGB64)
						err = suites.Iterate8Suite1()->iterate_generic(output->height, &i_data, CopyWorld_Iterate<float, A_u_short>);
				}

				wsP->PF_DisposeWorld(in_data->effect_ref, temp_world);
			}
				
				
			PF_UNLOCK_HANDLE(OCIO_data->u.arb_d.value);
			PF_UNLOCK_HANDLE(in_data->sequence_data);
			
			// to force a UI refresh after pixels have been sampled - might want to do this less often
			//out_data->out_flags |= PF_OutFlag_REFRESH_UI;
		}
	}

	
	if(wsP)
		in_data->pica_basicP->ReleaseSuite(kPFWorldSuite, kPFWorldSuiteVersion2);
		


	return err;
}

static PF_Err
SmartRender(
	PF_InData				*in_data,
	PF_OutData				*out_data,
	PF_SmartRenderExtra		*extra)

{
	PF_Err			err		= PF_Err_NONE,
					err2 	= PF_Err_NONE;
					
	PF_EffectWorld *input, *output;
	
	PF_ParamDef OCIO_data;

	// zero-out parameters
	AEFX_CLR_STRUCT(OCIO_data);
	
	
	// checkout input & output buffers.
	ERR(	extra->cb->checkout_layer_pixels( in_data->effect_ref, OCIO_INPUT, &input)	);
	ERR(	extra->cb->checkout_output(	in_data->effect_ref, &output)	);


	// bail before param checkout
	if(err)
		return err;

#define PF_CHECKOUT_PARAM_NOW( PARAM, DEST )	PF_CHECKOUT_PARAM(	in_data, (PARAM), in_data->current_time, in_data->time_step, in_data->time_scale, DEST )

	// checkout the required params
	ERR(	PF_CHECKOUT_PARAM_NOW( OCIO_DATA,	&OCIO_data )	);

	ERR(DoRender(	in_data, 
					input, 
					&OCIO_data,
					out_data, 
					output));

	// Always check in, no matter what the error condition!
	ERR2(	PF_CHECKIN_PARAM(in_data, &OCIO_data )	);


	return err;
  
}


DllExport	
PF_Err 
PluginMain (	
	PF_Cmd			cmd,
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	void			*extra)
{
	PF_Err		err = PF_Err_NONE;
	
	try	{
		switch (cmd) {
			case PF_Cmd_ABOUT:
				err = About(in_data,out_data,params,output);
				break;
			case PF_Cmd_GLOBAL_SETUP:
				err = GlobalSetup(in_data,out_data,params,output);
				break;
			case PF_Cmd_PARAMS_SETUP:
				err = ParamsSetup(in_data,out_data,params,output);
				break;
			case PF_Cmd_SEQUENCE_SETUP:
			case PF_Cmd_SEQUENCE_RESETUP:
				err = SequenceSetup(in_data, out_data, params, output);
				break;
			case PF_Cmd_SEQUENCE_FLATTEN:
				err = SequenceFlatten(in_data, out_data, params, output);
				break;
			case PF_Cmd_SEQUENCE_SETDOWN:
				err = SequenceSetdown(in_data, out_data, params, output);
				break;
			case PF_Cmd_SMART_PRE_RENDER:
				err = PreRender(in_data, out_data, (PF_PreRenderExtra*)extra);
				break;
			case PF_Cmd_SMART_RENDER:
				err = SmartRender(in_data, out_data, (PF_SmartRenderExtra*)extra);
				break;
			case PF_Cmd_EVENT:
				err = HandleEvent(in_data, out_data, params, output, (PF_EventExtra	*)extra);
				break;
			case PF_Cmd_DO_DIALOG:
				//err = DoDialog(in_data, out_data, params, output);
				break;	
			case PF_Cmd_ARBITRARY_CALLBACK:
				err = HandleArbitrary(in_data, out_data, params, output, (PF_ArbParamsExtra	*)extra);
				break;
		}
	}
	catch(PF_Err &thrown_err) { err = thrown_err; }
	catch(...) { err = PF_Err_INTERNAL_STRUCT_DAMAGED; }
	
	return err;
}
