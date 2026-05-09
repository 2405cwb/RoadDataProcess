// Copyright (C) 2002-2010 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CCameraSceneNode.h"
#include "ISceneManager.h"
#include "IVideoDriver.h"
#include "os.h"
#define Pi        3.14159265
#define PiOver180   1.74532925199433E-002
#define PiUnder180    5.72957795130823E+001
#define SMALL_NUMBER  0.00001
#define LARGE_NUMBER  1E20
namespace irr
{
	namespace scene
	{

		// 蔡红云 增加几个静态函数转换角度
		static inline double Radiansf(double Angle)
		{
			double r = (double)Angle*PiOver180; 
			return r;
		}

		static inline double Degreesf(double Angle)
		{
			double d = (double)Angle*PiUnder180;
			return d;
		}

		static inline double Diff(double a, double b)
		{
			if(a >= 0 && b >= 0)
			{
				if(a > b)
					return a-b;
				else
					return b-a;
			}
			if(a < 0 && b < 0)
			{
				if( a > b)
					return b-a;
				else
					return a-b;
			}
			if(a >= 0 && b < 0)
				return a-b;
			else
				return b-a;
		}
		//! constructor
		CCameraSceneNode::CCameraSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id,
			const core::vector3df& position, const core::vector3df& lookat)
			: ICameraSceneNode(parent, mgr, id, position),
			Target(lookat), UpVector(0.0f, 1.0f, 0.0f), ZNear(1.0f), ZFar(3000.0f),
			InputReceiverEnabled(true), TargetAndRotationAreBound(false)
		{
#ifdef _DEBUG
			setDebugName("CCameraSceneNode");
#endif
			// set default projection
			Fovy = core::PI / 2.5f;	// Field of view, in radians.

			const video::IVideoDriver* const d = mgr?mgr->getVideoDriver():0;
			if (d)
				Aspect = (f32)d->getCurrentRenderTargetSize().Width /
				(f32)d->getCurrentRenderTargetSize().Height;
			else
				Aspect = 4.0f / 3.0f;	// Aspect ratio.

			if (d)
			{
				WidthOfViewVolume = (f32)d->getCurrentRenderTargetSize().Width;
				HeightOfViewVolume = (f32)d->getCurrentRenderTargetSize().Height;
			}
			else
			{
				WidthOfViewVolume = 800;
				HeightOfViewVolume = 600;
			}

			if (WidthOfViewVolume == 0 || HeightOfViewVolume == 0)
			{
				WidthOfViewVolume = 800;
				HeightOfViewVolume = 600;
			}

			recalculateProjectionMatrix();
			recalculateViewArea();
		}


		//! Disables or enables the camera to get key or mouse inputs.
		void CCameraSceneNode::setInputReceiverEnabled(bool enabled)
		{
			InputReceiverEnabled = enabled;
		}


		//! Returns if the input receiver of the camera is currently enabled.
		bool CCameraSceneNode::isInputReceiverEnabled() const
		{
			_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
			return InputReceiverEnabled;
		}


		//! Sets the projection matrix of the camera.
		/** The core::matrix4 class has some methods
		to build a projection matrix. e.g: core::matrix4::buildProjectionMatrixPerspectiveFovLH
		\param projection: The new projection matrix of the camera. */
		void CCameraSceneNode::setProjectionMatrix(const core::matrix4& projection, bool isOrthogonal)
		{
			IsOrthogonal = isOrthogonal;
			ViewArea.getTransform ( video::ETS_PROJECTION ) = projection;
		}


		//! Gets the current projection matrix of the camera
		//! \return Returns the current projection matrix of the camera.
		const core::matrix4& CCameraSceneNode::getProjectionMatrix() const
		{
			return ViewArea.getTransform ( video::ETS_PROJECTION );
		}


		//! Gets the current view matrix of the camera
		//! \return Returns the current view matrix of the camera.
		const core::matrix4& CCameraSceneNode::getViewMatrix() const
		{
			return ViewArea.getTransform ( video::ETS_VIEW );
		}


		//! Sets a custom view matrix affector. The matrix passed here, will be
		//! multiplied with the view matrix when it gets updated.
		//! This allows for custom camera setups like, for example, a reflection camera.
		/** \param affector: The affector matrix. */
		void CCameraSceneNode::setViewMatrixAffector(const core::matrix4& affector)
		{
			Affector = affector;
		}


		//! Gets the custom view matrix affector.
		const core::matrix4& CCameraSceneNode::getViewMatrixAffector() const
		{
			return Affector;
		}


		//! It is possible to send mouse and key events to the camera. Most cameras
		//! may ignore this input, but camera scene nodes which are created for
		//! example with scene::ISceneManager::addMayaCameraSceneNode or
		//! scene::ISceneManager::addFPSCameraSceneNode, may want to get this input
		//! for changing their position, look at target or whatever.
		bool CCameraSceneNode::OnEvent(const SEvent& event)
		{
			if (!InputReceiverEnabled)
				return false;

			// send events to event receiving animators

			ISceneNodeAnimatorList::Iterator ait = Animators.begin();

			for (; ait != Animators.end(); ++ait)
				if ((*ait)->isEventReceiverEnabled() && (*ait)->OnEvent(event))
					return true;

			// if nobody processed the event, return false
			return false;
		}


		//! sets the look at target of the camera
		//! \param pos: Look at target of the camera.
		void CCameraSceneNode::setTarget(const core::vector3df& pos)
		{
			Target = pos;
			// 旋转中心默认是Target
			m_rotCenter = pos;

			if(TargetAndRotationAreBound)
			{
				const core::vector3df toTarget = Target - getAbsolutePosition();
				ISceneNode::setRotation(toTarget.getHorizontalAngle());
			}
		}


		//! Sets the rotation of the node.
		/** This only modifies the relative rotation of the node.
		If the camera's target and rotation are bound ( @see bindTargetAndRotation() )
		then calling this will also change the camera's target to match the rotation.
		\param rotation New rotation of the node in degrees. */
		void CCameraSceneNode::setRotation(const core::vector3df& rotation)
		{
			if(TargetAndRotationAreBound)
				Target = getAbsolutePosition() + rotation.rotationToDirection();

			ISceneNode::setRotation(rotation);
		}


		//! Gets the current look at target of the camera
		//! \return Returns the current look at target of the camera
		const core::vector3df& CCameraSceneNode::getTarget() const
		{
			return Target;
		}


		//! sets the up vector of the camera
		//! \param pos: New upvector of the camera.
		void CCameraSceneNode::setUpVector(const core::vector3df& pos)
		{
			UpVector = pos;
		}


		//! Gets the up vector of the camera.
		//! \return Returns the up vector of the camera.
		const core::vector3df& CCameraSceneNode::getUpVector() const
		{
			return UpVector;
		}


		f32 CCameraSceneNode::getNearValue() const
		{
			return ZNear;
		}


		f32 CCameraSceneNode::getFarValue() const
		{
			return ZFar;
		}


		f32 CCameraSceneNode::getAspectRatio() const
		{
			return Aspect;
		}


		f32 CCameraSceneNode::getFOV() const
		{
			return Fovy;
		}

		void CCameraSceneNode::setProjectionType(u32 type)
		{
			//默认情况下设置为透视投影
			if (type == 0)
			{
				core::matrix4 mat;
				mat.buildProjectionMatrixPerspectiveFovRH(Fovy, Aspect, ZNear, ZFar);
				setProjectionMatrix(mat,false);
				IsOrthogonal = false;
			}
			else
			{
				core::matrix4 mat;
				mat.buildProjectionMatrixOrthoRH(WidthOfViewVolume, HeightOfViewVolume, ZNear, ZFar);
				setProjectionMatrix(mat,true);
				IsOrthogonal = true;
			}
		}

		void CCameraSceneNode::setNearValue(f32 f)
		{
			ZNear = f;
			recalculateProjectionMatrix();
		}


		void CCameraSceneNode::setFarValue(f32 f)
		{
			ZFar = f;
			recalculateProjectionMatrix();
		}


		void CCameraSceneNode::setAspectRatio(f32 f)
		{
			Aspect = f;
			recalculateProjectionMatrix();
		}


		void CCameraSceneNode::setFOV(f32 f)
		{
			Fovy = f;
			recalculateProjectionMatrix();
		}

		void CCameraSceneNode::setWidthofViewVolume(f32 width)
		{
			WidthOfViewVolume = width;
			recalculateProjectionMatrix();
		}

		void CCameraSceneNode::setHeightofViewVolume(f32 height)
		{
			HeightOfViewVolume = height;
			recalculateProjectionMatrix();
		}

		f32 CCameraSceneNode::getWidthofViewVolume()
		{
			return WidthOfViewVolume;
		}

		f32 CCameraSceneNode::getHeightofViewVolume()
		{
			return HeightOfViewVolume;
		}
		void CCameraSceneNode::recalculateProjectionMatrix()
		{
			if (!IsOrthogonal)
			{	
				ViewArea.getTransform ( video::ETS_PROJECTION ).buildProjectionMatrixPerspectiveFovRH(Fovy, Aspect, ZNear, ZFar);
			}
			else
			{
				ViewArea.getTransform ( video::ETS_PROJECTION ).buildProjectionMatrixOrthoRH(WidthOfViewVolume, HeightOfViewVolume, ZNear, ZFar);
			}
		}


		//! prerender
		void CCameraSceneNode::OnRegisterSceneNode()
		{
			if ( SceneManager->getActiveCamera () == this )
				SceneManager->registerNodeForRendering(this, ESNRP_CAMERA);

			ISceneNode::OnRegisterSceneNode();
		}


		//! render
		void CCameraSceneNode::render()
		{
			core::vector3df pos = getAbsolutePosition();
			core::vector3df tgtv = Target - pos;
			tgtv.normalize();

			// if upvector and vector to the target are the same, we have a
			// problem. so solve this problem:
			core::vector3df up = UpVector;
			up.normalize();

			f32 dp = tgtv.dotProduct(up);

			if ( core::equals(core::abs_<f32>(dp), 1.f) )
			{
				up.X += 0.5f;
			}

			ViewArea.getTransform(video::ETS_VIEW).buildCameraLookAtMatrixRH(pos, Target, up);
			ViewArea.getTransform(video::ETS_VIEW) *= Affector;
			recalculateViewArea();

			video::IVideoDriver* driver = SceneManager->getVideoDriver();
			if ( driver)
			{
				driver->setTransform(video::ETS_PROJECTION, ViewArea.getTransform ( video::ETS_PROJECTION) );
				driver->setTransform(video::ETS_VIEW, ViewArea.getTransform ( video::ETS_VIEW) );
			}
		}


		//! returns the axis aligned bounding box of this node
		const core::aabbox3d<f32>& CCameraSceneNode::getBoundingBox() const
		{
			return ViewArea.getBoundingBox();
		}


		//! returns the view frustum. needed sometimes by bsp or lod render nodes.
		const SViewFrustum* CCameraSceneNode::getViewFrustum() const
		{
			return &ViewArea;
		}


		void CCameraSceneNode::recalculateViewArea()
		{
			ViewArea.cameraPosition = getAbsolutePosition();

			core::matrix4 m(core::matrix4::EM4CONST_NOTHING);
			m.setbyproduct_nocheck(ViewArea.getTransform(video::ETS_PROJECTION),
				ViewArea.getTransform(video::ETS_VIEW));
			ViewArea.setFrom(m);
		}


		//! Writes attributes of the scene node.
		void CCameraSceneNode::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
		{
			ICameraSceneNode::serializeAttributes(out, options);

			out->addVector3d("Target", Target);
			out->addVector3d("UpVector", UpVector);
			out->addFloat("Fovy", Fovy);
			out->addFloat("Aspect", Aspect);
			out->addFloat("ZNear", ZNear);
			out->addFloat("ZFar", ZFar);
			out->addBool("Binding", TargetAndRotationAreBound);
			out->addBool("ReceiveInput", InputReceiverEnabled);
		}

		//! Reads attributes of the scene node.
		void CCameraSceneNode::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
		{
			ICameraSceneNode::deserializeAttributes(in, options);

			Target = in->getAttributeAsVector3d("Target");
			UpVector = in->getAttributeAsVector3d("UpVector");
			Fovy = in->getAttributeAsFloat("Fovy");
			Aspect = in->getAttributeAsFloat("Aspect");
			ZNear = in->getAttributeAsFloat("ZNear");
			ZFar = in->getAttributeAsFloat("ZFar");
			TargetAndRotationAreBound = in->getAttributeAsBool("Binding");
			if ( in->findAttribute("ReceiveInput") )
				InputReceiverEnabled = in->getAttributeAsBool("InputReceiverEnabled");

			recalculateProjectionMatrix();
			recalculateViewArea();
		}


		//! Set the binding between the camera's rotation adn target.
		void CCameraSceneNode::bindTargetAndRotation(bool bound)
		{
			TargetAndRotationAreBound = bound;
		}


		//! Gets the binding between the camera's rotation and target.
		bool CCameraSceneNode::getTargetAndRotationBinding(void) const
		{
			return TargetAndRotationAreBound;
		}


		//! Creates a clone of this scene node and its children.
		ISceneNode* CCameraSceneNode::clone(ISceneNode* newParent, ISceneManager* newManager)
		{
			ICameraSceneNode::clone(newParent, newManager);

			if (!newParent)
				newParent = Parent;
			if (!newManager)
				newManager = SceneManager;

			CCameraSceneNode* nb = new CCameraSceneNode(newParent,
				newManager, ID, RelativeTranslation, Target);

			nb->ISceneNode::cloneMembers(this, newManager);
			nb->ICameraSceneNode::cloneMembers(this);

			nb->Target = Target;
			nb->UpVector = UpVector;
			nb->Fovy = Fovy;
			nb->Aspect = Aspect;
			nb->ZNear = ZNear;
			nb->ZFar = ZFar;
			nb->ViewArea = ViewArea;
			nb->Affector = Affector;
			nb->InputReceiverEnabled = InputReceiverEnabled;
			nb->TargetAndRotationAreBound = TargetAndRotationAreBound;

			if ( newParent )
				nb->drop();
			return nb;
		}
		// 计算pos和target之间距离
		void CCameraSceneNode::CalculateFocalLength()
		{
			core::vector3df pos = getPosition();
			f32 temp = (pos-Target).getLength();
			if(temp < 0.0001f)
				m_fFocalLength = 1.0f;
			else
				m_fFocalLength = temp;

		}
		// 返回pos和target之间距离
		double CCameraSceneNode::GetFocalLength()
		{
			return m_fFocalLength;
		}
		// 获取pos和target构成的向量在X，Y，Z轴的夹角
		void CCameraSceneNode::GetRotationAboutTarget(f32& x,f32& y, f32& z)
		{
			CalculateFocalLength();

			f32 ax, ay, az;
			core::vector3df pos = getPosition();
			ax = pos.X - Target.X;
			ay = pos.Y- Target.Y;
			az = pos.Z - Target.Z;

			// Compensate for rounding errors
			if(fabs(ax) < SMALL_NUMBER)
				ax = 0.0f;
			if(fabs(ay) < SMALL_NUMBER)
				ay = 0.0f;
			if(fabs(az) < SMALL_NUMBER)
				az = 0.0f;

			// Calculate the Camera Pitch angle
			if(UpVector.Z >= 0)

			{
				// 求取pos和target之间的矢量和X轴的夹角，不直接求取该矢量和X轴
				// 单位向量夹角的原因是，假如矢量一直在ZOY平面内变动，那么该夹角
				// 就会是一直保持90度或者-90度。那么就会对旋转矩阵构成不了变换
				m_fPitch = Degreesf(asin(-az/m_fFocalLength));

				// 防止俯视图跳变、以及相机飞走。。
				if (az == m_fFocalLength  )
				{
					m_fPitch = -90;
				}

			}
			else
				m_fPitch = 180-Degreesf(asin(-az/m_fFocalLength));

			// Calculate the Camera 'Yaw' angle
			if(m_fPitch == 90.0f)
				m_fYaw = Degreesf(atan2(-UpVector.X, -UpVector.Y));
			else if(m_fPitch == -90.0f)
				m_fYaw = Degreesf(atan2(UpVector.X, UpVector.Y));
			else if(m_fPitch < 90.0f)
				m_fYaw = Degreesf(atan2(-ax, -ay));
			else
				m_fYaw = Degreesf(atan2(ax, ay));
			if (m_fRoll<-360.0f)
			{
				m_fRoll = 0.0f;
			}
			// Pass the values back to the caller
			x = m_fPitch;
			y = m_fRoll;
			z = m_fYaw;
		}

		// 设置pos和target构成的向量在X，Y，Z轴的夹角，进而构建旋转矩阵
		// 对pos进行变换
		void CCameraSceneNode::SetRotationAboutTarget(f32 x, f32 y, f32 z)
		{
			core::vector3df pos;
			core::matrix4 mattest;
			// costruct matrix
			mattest.setRotationDegrees(core::vector3df(x, 0, -z));

			core::vector3df Eyett(0,-m_fFocalLength,0);
			mattest.transformVect(Eyett);

			pos.X = Eyett.X + Target.X;
			pos.Y = Eyett.Y + Target.Y;
			pos.Z = Eyett.Z + Target.Z;

			// Compensate for rounding errors
			if(fabs(pos.X) < SMALL_NUMBER)
				pos.X = 0.0f;
			if(fabs(pos.Y) < SMALL_NUMBER)
				pos.Y= 0.0f;
			if(fabs(pos.Z) < SMALL_NUMBER)
				pos.Z = 0.0f;

			setPosition(pos);
			// Calculate our camera's UpVector using ONLY the 'X' (Pitch) and 'Z' (Yaw)
			core::vector3df upvetortst(0, 0, 1);
			mattest.transformVect(upvetortst);
			// Compensate for rounding errors
			if(fabs(upvetortst.X) < SMALL_NUMBER)
				upvetortst.X = 0.0f;
			if(fabs(upvetortst.Y) < SMALL_NUMBER)
				upvetortst.Y = 0.0f;
			if(fabs(upvetortst.Z) < SMALL_NUMBER)
				upvetortst.Z = 0.0f;
			UpVector= upvetortst;
			// Just save the 'y' value for later use when calculating the camera's 'Up' vector
			// prior to calling gluLookAt()
			m_fRoll = y;
		}

	} // end namespace
} // end namespace

