#pragma once
#include "sdlgf.h"
#include "Extras.h"
#include "Player.h"
#include "GameItems.h"
#include <memory>
#include <vector>

namespace Demo {
	constexpr float UNSPECIFIED = -1;

	enum class ProjectileBehavior : unsigned char {
		Straight,
		Homing,
		SineWave,
		Spiral,
		Boomerang
	};

	// When an airburst projectile tears itself into a ring of children.
	// OnArrival fires the moment the projectile crosses its target point,
	// OnDecay fires when its decay timer runs out (also the trigger every
	// chained generation uses, since children have no target of their own).
	enum class ProjectileSplitTrigger : unsigned char {
		None,
		OnArrival,
		OnDecay
	};

	// Plain-data spawn description. The fluent setters mirror the old
	// per-class builders so spawn sites read the same as before.
	struct ProjectileDesc {
		// Render
		sdlgf::Texture* texture = nullptr;
		std::vector<RECT> frames; // empty = draw the whole texture
		unsigned int frameRate = 12;
		float spriteOriginX = 0.f;
		float spriteOriginY = 0.f;
		sdlgf::Texture* shardTexture = nullptr; // optional override for the projectile's split children
		std::vector<RECT> shardFrames; // empty = draw the whole shard texture
		unsigned int shardFrameRate = 12;
		float shardOriginX = 0.f;
		float shardOriginY = 0.f;
		// Placement / collision (ellipse, origin at center)
		float x = 0.f;
		float y = 0.f;
		float colliderWidth = 0.f;
		float colliderHeight = 0.f;
		float shardColliderWidth = 0.f;
		float shardColliderHeight = 0.f;
		// Common
		ProjectileBehavior behavior = ProjectileBehavior::Straight;
		sdlgf::Vec2 trajectory{ 1.f, 0.f };
		float velocity = 0.f;
		float delay = 0.f;
		float decayTime = UNSPECIFIED;
		float damage = 0.f;
		// Homing
		float turnSpeed = 0.f;
		// Sine wave / zigzag
		float amplitude = 0.f;
		float frequency = 0.f;
			bool zigzag = false;
		// Spiral
		float initialAngle = 0.f;
		float radialSpeed = 0.f;
		float angularVelocity = 0.f;
		// Boomerang
		float returnAcceleration = 0.f;
		// Acceleration along the heading (units/s^2) for Straight, Homing and SineWave shots.
		// Speed stops changing at velocityLimit; a decelerating shot with no limit stops at 0
		// instead of reversing (that is what Boomerang is for).
		float acceleration = 0.f;
		float velocityLimit = UNSPECIFIED;
		// Airburst: the projectile bursts into a ring of children that inherit its
		// sprite, collider and status effect, then travel straight outward.
		ProjectileSplitTrigger splitTrigger = ProjectileSplitTrigger::None;
		int splitCount = 0;
		float splitTargetX = 0.f;
		float splitTargetY = 0.f;
		float splitVelocity = 0.f;
		float splitSpread = 0.f;       // arc in radians; 0 = evenly spaced full circle
		float splitAngleOffset = 0.f;  // rotates the ring off the parent's heading
		bool splitRandomizeAngle = false;
		float splitDecayTime = UNSPECIFIED;
		float splitDamage = UNSPECIFIED; // UNSPECIFIED = inherit the parent's damage
		int splitGenerations = 1;      // >1 lets the children burst again on decay
		float splitArrivalHoldTime = 0.f;
		// Ghost trail (afterimages reusing the projectile's own texture)
		sdlgf::Texture* ghostTexture = nullptr;
		RECT ghostSrcRect{};
		bool ghostHasSrcRect = false;
		float ghostOriginX = 0.f;
		float ghostOriginY = 0.f;
		// Status effect on hit
		bool hasStatusEffect = false;
		bool randomizeStatusEffect = false;
		ModifierType statusEffectType = ModifierType::Freeze;
		float statusEffectValue = 0.f;
		int statusEffectDuration = 0;
		ModifierType altStatusEffectType = ModifierType::Burn;
		float altStatusEffectValue = 0.f;
		int altStatusEffectDuration = 0;

		ProjectileDesc(
			sdlgf::Texture* texture,
			float spriteOriginX,
			float spriteOriginY,
			float colliderWidth,
			float colliderHeight,
			float x,
			float y
		) : texture(texture),
			spriteOriginX(spriteOriginX),
			spriteOriginY(spriteOriginY),
			colliderWidth(colliderWidth),
			colliderHeight(colliderHeight),
			x(x),
			y(y) {}
		ProjectileDesc(
			sdlgf::Texture* texture,
			std::vector<RECT> frames,
			unsigned int frameRate,
			float spriteOriginX,
			float spriteOriginY,
			float colliderWidth,
			float colliderHeight,
			float x,
			float y
		) : texture(texture),
			frames(std::move(frames)),
			frameRate(frameRate),
			spriteOriginX(spriteOriginX),
			spriteOriginY(spriteOriginY),
			colliderWidth(colliderWidth),
			colliderHeight(colliderHeight),
			x(x),
			y(y) {}

		ProjectileDesc& SetTrajectory(sdlgf::Vec2 trajectory);
		ProjectileDesc& SetTargetPosition(float targetX, float targetY);
		ProjectileDesc& SetVelocity(float velocity);
		ProjectileDesc& SetDelay(float delay);
		ProjectileDesc& SetDecayTime(float decayTime);
		ProjectileDesc& SetDamage(float damage);
		ProjectileDesc& SetHoming(float turnSpeed);
		ProjectileDesc& SetWave(float amplitude, float frequency, bool zigzag = false);
		ProjectileDesc& SetSpiralParams(float initialAngle, float radialSpeed, float angularVelocity);
		ProjectileDesc& SetInitialVelocity(float velocity);
		ProjectileDesc& SetReturnAcceleration(float acceleration);
		ProjectileDesc& SetAcceleration(float acceleration, float velocityLimit = UNSPECIFIED);
		// Aims the projectile at (targetX, targetY) and arms it to burst there.
		ProjectileDesc& SetSplitOnArrival(float targetX, float targetY, int count, float velocity);
		// Bursts wherever the projectile happens to be when its decay timer expires.
		ProjectileDesc& SetSplitOnDecay(int count, float velocity);
		ProjectileDesc& SetSplitSpread(float spread, float angleOffset = 0.f);
		ProjectileDesc& SetSplitRandomAngle(bool randomize = true);
		ProjectileDesc& SetSplitDecayTime(float decayTime);
		ProjectileDesc& SetSplitDamage(float damage);
		ProjectileDesc& SetSplitGenerations(int generations);
		ProjectileDesc& SetSplitHoldTime(float holdTime);
		ProjectileDesc& SetShardTexture(sdlgf::Texture* texture, std::vector<RECT> frames, unsigned int frameRate, float originX, float originY, float colliderWidth, float colliderHeight);
		ProjectileDesc& SetShardTexture(sdlgf::Texture* texture, float originX, float originY, float colliderWidth, float colliderHeight);
		ProjectileDesc& SetGhostSprite(sdlgf::Texture* texture, RECT srcRect, float originX, float originY);
		ProjectileDesc& SetStatusEffect(ModifierType type, float value, int duration);
		ProjectileDesc& SetRandomStatusEffect(
			ModifierType typeA, float valueA, int durationA,
			ModifierType typeB, float valueB, int durationB
		);
	};

	// A laser is a stationary beam rather than a travelling bullet: it telegraphs
	// along its line for warnTime, then turns lethal for fireTime before vanishing.
	// The system owns the blink, the beam rendering and the damage, so a pattern
	// only describes the geometry and how long each phase lasts.
	struct LaserDesc {
		// Placement (the collider is an oriented box centred on x/y)
		float x = 0.f;
		float y = 0.f;
		float angle = 0.f;         // radians; 0 = the beam runs along +X
		float length = 0.f;
		float thickness = 4.f;     // lethal core; the glow is drawn wider
		// Timing
		float delay = 0.f;
		float warnTime = 1.5f;
		float fireTime = 1.f;
		float damage = 0.f;
		// Render
		float warnThickness = 2.f;
		float glowThickness = 16.f;
		sdlgf::Color warnColor = COLOR_ARGB(150, 255, 100, 100);
		sdlgf::Color glowColor = COLOR_ARGB(120, 255, 20, 147);
		sdlgf::Color coreColor = 0xFFFFFFFF;
		// Status effect on hit
		bool hasStatusEffect = false;
		bool randomizeStatusEffect = false;
		ModifierType statusEffectType = ModifierType::Freeze;
		float statusEffectValue = 0.f;
		int statusEffectDuration = 0;
		ModifierType altStatusEffectType = ModifierType::Burn;
		float altStatusEffectValue = 0.f;
		int altStatusEffectDuration = 0;

		LaserDesc(float x, float y, float angle, float length)
			: x(x), y(y), angle(angle), length(length) {}
		// Arena beams are nearly always axis aligned, so both spans are spelled out.
		static LaserDesc Vertical(float x, float centerY, float length);
		static LaserDesc Horizontal(float y, float centerX, float length);

		LaserDesc& SetThickness(float thickness);
		LaserDesc& SetDelay(float delay);
		LaserDesc& SetWarnTime(float warnTime);
		LaserDesc& SetFireTime(float fireTime);
		LaserDesc& SetDamage(float damage);
		LaserDesc& SetGlowThickness(float glowThickness);
		LaserDesc& SetColors(sdlgf::Color warnColor, sdlgf::Color glowColor, sdlgf::Color coreColor);
		LaserDesc& SetStatusEffect(ModifierType type, float value, int duration);
		LaserDesc& SetRandomStatusEffect(
			ModifierType typeA, float valueA, int durationA,
			ModifierType typeB, float valueB, int durationB
		);
	};

	// Data-oriented replacement for the old IProjectile class hierarchy.
	// Entities are dense indices into the parallel component arrays below;
	// destruction swap-pops, so indices never escape this class. Behaviors
	// are data (MotionComponent + switch), not virtual dispatch, and
	// rendering batches all projectiles that share a texture/frame set into
	// a single sprite Begin/End instead of one per projectile.
	class ProjectileSystem {
	private:
		struct RenderDesc {
			sdlgf::Texture* texture;
			std::vector<RECT> frames;
			unsigned int frameRate;
			float spriteOriginX, spriteOriginY;
		};
		struct TransformComponent {
			float x, y, rotation;
		};
		struct MotionComponent {
			ProjectileBehavior behavior;
			sdlgf::Vec2 trajectory;
			float velocity;
			float turnSpeed;
			float amplitude, frequency;
				bool zigzag = false;
			float baseX, baseY;      // sine wave carrier / spiral origin
			float radius, angle;     // spiral state
			float radialSpeed, angularVelocity;
			float returnAcceleration;
			float acceleration;
			float velocityLimit = UNSPECIFIED;
		};
		struct LifetimeComponent {
			float delay, elapsed, decayTime;
			bool arrivalHeld = false;
			float holdRemaining = 0.f;
			bool launched = false; // delay finished and the projectile has started moving
		};
		struct CombatComponent {
			float damage;
			float colliderWidth, colliderHeight;
			bool destroyOnHit;
			bool hasStatusEffect;
			bool randomizeStatusEffect;
			ModifierType statusEffectType;
			float statusEffectValue;
			int statusEffectDuration;
			ModifierType altStatusEffectType;
			float altStatusEffectValue;
			int altStatusEffectDuration;
			bool statusApplied = false;
			// Indicator colours, resolved once at spawn from the status effect(s). The alt colour
			// only differs from the main one for a randomized effect, where the two alternate.
			sdlgf::Color statusColor = 0xFFFFFFFF;
			sdlgf::Color altStatusColor = 0xFFFFFFFF;
		};
		struct SplitComponent {
			ProjectileSplitTrigger trigger;
			unsigned int batchIndex;
			int count;
			float targetX, targetY;
			float velocity;
			float spread, angleOffset;
			bool randomizeAngle;
			float decayTime;
			float damage;
			int generations;
			float colliderWidth, colliderHeight;
			float holdTime = 0.f;
		};
		struct RenderComponent {
			unsigned int batchIndex;
			unsigned int frameIndex;
			unsigned long long frameDelta; // ms accumulator, mirrors AnimatedSprite
			bool visibleDuringDelay;
		};

		// Lasers sit beside the projectiles instead of inside the component arrays:
		// they never move, they collide as an oriented box rather than an ellipse and
		// they are drawn as primitives rather than sprites. Combat data is shared, so
		// their damage and status effects behave exactly like a bullet's.
		struct LaserComponent {
			float x, y, angle, length;
			float thickness, warnThickness, glowThickness;
			float delay, warnTime, fireTime, elapsed;
			sdlgf::Color warnColor, glowColor, coreColor;
		};

		// One batch per unique (texture, frames, frame rate, origin); every
		// projectile in a batch is drawn through the same sprite object.
		struct SpriteBatch {
			sdlgf::Texture* texture = nullptr;
			std::vector<RECT> frames;
			unsigned int frameRate = 12;
			float originX = 0.f;
			float originY = 0.f;
			std::unique_ptr<sdlgf::StaticSprite> sprite;
		};

		// Components (parallel arrays; index = entity)
		std::vector<TransformComponent> transforms;
		std::vector<MotionComponent> motions;
		std::vector<LifetimeComponent> lifetimes;
		std::vector<CombatComponent> combats;
		std::vector<RenderComponent> renders;
		std::vector<SplitComponent> splits;
		std::vector<std::unique_ptr<sdlgf::ParticleSystem>> emitters;
		std::vector<unsigned char> dead;

		std::vector<LaserComponent> lasers;
		std::vector<CombatComponent> laserCombats;
		std::vector<unsigned char> laserDead;

		// A burst snapshots everything its children need, so the children can be
		// created after the update loop has finished. Spawning them inline would
		// push onto the component arrays the loop is iterating and invalidate the
		// references it holds.
		struct PendingBurst {
			float x, y;
			float baseAngle;
			SplitComponent split;
			CombatComponent combat;
			unsigned int batchIndex;
		};
		std::vector<PendingBurst> pendingBursts;

		// Patterns spawn many projectiles at once; cooldowns keep their sounds from stacking.
		float spawnSfxCooldown = 0.f;
		float launchSfxCooldown = 0.f;
		float sfxScale = 1.f; // multiplies spawn/launch volume; 0 mutes them

		std::vector<SpriteBatch> batches;
		std::weak_ptr<Player> target;
		ICombatant* owner = nullptr; // who fired these; credited as the source of any status they apply
		std::shared_ptr<sdlgf::Texture> trailTexture; // shared 4x4 white square

		// Create a new batch if no existing batch matches the projectile's texture, frames, frame rate and origin.
		unsigned int GetOrCreateBatch(const RenderDesc& desc);
		static void ResolveStatusColors(CombatComponent& combat);
		void ApplyHit(Player& player, CombatComponent& combat);
		void DestroyAt(size_t index);
		void DestroyLaserAt(size_t index);
		void QueueBurst(size_t index);
		void EmitBurst(const PendingBurst& burst);
	public:
		void SetOwner(ICombatant* newOwner) { owner = newOwner; }
		// Patterns that fire hundreds of bullets turn their spawn sounds down so they do not drone.
		void SetSfxScale(float scale) { sfxScale = scale; }
		void Spawn(const std::shared_ptr<Player>& player, const ProjectileDesc& desc);
		void Spawn(const std::shared_ptr<Player>& player, const LaserDesc& desc);
		void Update(unsigned long long deltaTime);
		void Draw(sdlgf::GraphicsDevice* graphicsDevice, const sdlgf::Camera& camera, unsigned long long deltaTime);
		bool IsEmpty() const { return transforms.empty() && lasers.empty(); }
		size_t Count() const { return transforms.size() + lasers.size(); }
	};
}
